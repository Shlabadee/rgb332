#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "argparse.h"
#include "common.h"
#include "ezpng.h"

// clang-format off
static const char* const usage[] =
{
    "rgb332enc [options]",
    NULL
};
// clang-format on

void ezpng_print_error(const char* msg)
{
	fprintf(stderr, "%s. Reason: %s\n", msg, ezpng_get_error());
}

static inline float l_roundf(float n)
{
	return (float)((int)(n + 0.5f));
}

static inline float l_clampf(float v, float lo, float hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

RGBF* RGBF_get(RGBF* input_image, int width, int height, int x, int y)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return NULL;

	return &input_image[(y * width) + x];
}

RGBF RGBF_copy(RGBF* src)
{
	RGBF copy;
	memcpy(&copy, src, sizeof(RGBF));
	return copy;
}

RGBF find_closest_limited_color(RGBF* oldpixel, bool round)
{
	RGBF newpixel;

	newpixel.r = l_roundf((oldpixel->r / 255.f) * r_maxf);
	newpixel.g = l_roundf((oldpixel->g / 255.f) * g_maxf);
	newpixel.b = l_roundf((oldpixel->b / 255.f) * b_maxf);

	if (round)
	{
		newpixel.r = l_roundf(newpixel.r);
		newpixel.g = l_roundf(newpixel.g);
		newpixel.b = l_roundf(newpixel.b);
	}

	return newpixel;
}

RGBF promote_pixel(RGBF* oldpixel, bool round)
{
	RGBF newpixel;

	newpixel.r = (oldpixel->r / r_maxf) * 255.f;
	newpixel.g = (oldpixel->g / g_maxf) * 255.f;
	newpixel.b = (oldpixel->b / b_maxf) * 255.f;

	if (round)
	{
		newpixel.r = l_roundf(newpixel.r);
		newpixel.g = l_roundf(newpixel.g);
		newpixel.b = l_roundf(newpixel.b);
	}

	return newpixel;
}

RGBF RGBF_add(RGBF* a, RGBF* b)
{
	RGBF c;

	c.r = a->r + b->r;
	c.g = a->g + b->g;
	c.b = a->b + b->b;

	return c;
}

RGBF RGBF_subtract(RGBF* a, RGBF* b)
{
	RGBF c;

	c.r = a->r - b->r;
	c.g = a->g - b->g;
	c.b = a->b - b->b;

	return c;
}

RGBF RGBF_multiply(RGBF* a, float b)
{
	RGBF c;

	c.r = a->r * b;
	c.g = a->g * b;
	c.b = a->b * b;

	return c;
}

bool apply_dither(RGBF* working_image, RGBF* quant_error, float weight, int width, int height,
                  int x, int y)
{
	RGBF* current_pixel = RGBF_get(working_image, width, height, x, y);

	if (!current_pixel)
		return false;

	RGBF temp;
	temp = RGBF_multiply(quant_error, weight);
	temp = RGBF_add(&temp, current_pixel);
	temp.r = l_clampf(temp.r, 0.f, 255.f);
	temp.g = l_clampf(temp.g, 0.f, 255.f);
	temp.b = l_clampf(temp.b, 0.f, 255.f);
	*current_pixel = temp;

	return true;
}

int main(int argc, const char** argv)
{
	printf("rgb332enc v1.0.1 - Shlabadee\n");
	const char* input_file_path = NULL;
	const char* output_file_path = NULL;
	int dither_input = 0;
	bool dither = false;
	int serpentine_input = 0;
	bool serpentine = false;
	int legal_input = 0;
	bool legal = false;

	RGBF* working_image;
	uint8_t* output_image;
	FILE* output_file;

	// clang-format off
	struct argparse_option options[] =
	{
		OPT_HELP(),
		OPT_STRING('i', "input", &input_file_path, "input file"),
		OPT_STRING('o', "output", &output_file_path, "output file"),
		OPT_BOOLEAN('d', "dither", &dither_input, "enable dithering"),
		OPT_BOOLEAN('s', "serpentine", &serpentine_input, "enable serpentine dithering"),
		OPT_BOOLEAN('l', "legal", &legal_input, "show legal information and exit"),
		OPT_END()
	};
	// clang-format on

	struct argparse argparse_instance;
	argparse_init(&argparse_instance, options, usage, 0);
	int parsed_argc = argparse_parse(&argparse_instance, argc, argv);

	if (argc == 1)
	{
		argparse_usage(&argparse_instance);
		return 0;
	}

	dither = dither_input == 1;
	serpentine = serpentine_input == 1;
	legal = legal_input == 1;

	if (legal)
	{
		print_legal();
		return 0;
	}

	// actual program start

	ezpng_decoder* input_file = ezpng_decoder_open(input_file_path);

	if (!input_file)
	{
		ezpng_print_error("Error opening input file");
		return 1;
	}

	int width, height;
	int resolution;

	width = ezpng_decoder_get_width(input_file);
	height = ezpng_decoder_get_height(input_file);
	resolution = width * height;

	const ezpng_rgba* input_file_image = ezpng_decoder_get_data(input_file);

	working_image = malloc(resolution * sizeof(*working_image));

	if (!working_image)
	{
		fprintf(stderr, "Unable to allocate memory for working image.\n");
		goto cleanup;
	}

	for (int i = 0; i < resolution; ++i)
	{
		working_image[i].r = (float)input_file_image[i].r;
		working_image[i].g = (float)input_file_image[i].g;
		working_image[i].b = (float)input_file_image[i].b;
	}

	ezpng_decoder_close(input_file);
	input_file = NULL;

	int direction = 1;

	for (int y = 0; y < height; ++y)
	{
		int x = direction == 1 ? 0 : width - 1;

		for (int i = 0; i < width; ++i)
		{
			RGBF* current_pixel = RGBF_get(working_image, width, height, x, y);

			if (!current_pixel)
			{
				fprintf(stderr, "FATAL: unable to get current pixel. (%i, %i)\n", x, y);
				goto cleanup;
			}

			RGBF oldpixel = RGBF_copy(current_pixel);
			RGBF newpixel = find_closest_limited_color(current_pixel, false);
			newpixel = promote_pixel(&newpixel, false);
			*current_pixel = newpixel;

			// skip dithering
			if (!dither)
				continue;

			RGBF quant_error = RGBF_subtract(&oldpixel, &newpixel);

			if (direction == 1)
			{
				apply_dither(working_image, &quant_error, 7.f / 16.f, width, height, x + 1, y);
				apply_dither(working_image, &quant_error, 3.f / 16.f, width, height, x - 1,
				             y + 1);
				apply_dither(working_image, &quant_error, 5.f / 16.f, width, height, x, y + 1);
				apply_dither(working_image, &quant_error, 1.f / 16.f, width, height, x + 1,
				             y + 1);
			}
			else
			{
				apply_dither(working_image, &quant_error, 7.f / 16.f, width, height, x - 1, y);
				apply_dither(working_image, &quant_error, 3.f / 16.f, width, height, x + 1,
				             y + 1);
				apply_dither(working_image, &quant_error, 5.f / 16.f, width, height, x, y + 1);
				apply_dither(working_image, &quant_error, 1.f / 16.f, width, height, x - 1,
				             y + 1);
			}

			x += direction;
		}

		if (serpentine)
			direction = -direction;
	}

	output_image = malloc(resolution);

	if (!output_image)
	{
		fprintf(stderr, "Error allocating memory for output image.\n");
		goto cleanup;
	}

	output_file = fopen(output_file_path, "wb");

	if (!output_file)
	{
		fprintf(stderr, "Unable to create output file.\n");
		goto cleanup;
	}

	for (int i = 0; i < resolution; ++i)
	{
		RGBF temp = find_closest_limited_color(&working_image[i], true);

		uint8_t r = (uint8_t)temp.r;
		uint8_t g = (uint8_t)temp.g;
		uint8_t b = (uint8_t)temp.b;

		output_image[i] = (r << 5) | (g << 2) | b;
	}

	fwrite(&I332_MAGIC_NUMBER, 4, 1, output_file);

	fwrite(&width, sizeof(width), 1, output_file);
	fwrite(&height, sizeof(height), 1, output_file);
	fwrite(output_image, sizeof(*output_image), resolution, output_file);

cleanup:
	if (output_file)
		fclose(output_file);

	if (output_image)
		free(output_image);

	if (working_image)
		free(working_image);

	if (input_file)
		ezpng_decoder_close(input_file);

	return 0;
}
