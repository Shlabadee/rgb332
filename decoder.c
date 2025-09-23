#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "argparse.h"
#include "common.h"
#include "ezpng.h"
#include "magic_number.h"

void print_error(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
}

// clang-format off
static const char* const usage[] =
{
	"rgb332dec [options]",
	NULL
};
// clang-format on

int main(int argc, const char** argv)
{
	printf("rgb332dec v1.0.0 - Shlabadee\n");
	const char* input_file_path = NULL;
	const char* output_file_path = NULL;
	bool legal;

	uint8_t* input_image = NULL;
	ezpng_rgba* output_image = NULL;

	// clang-format off
	struct argparse_option options[] =
	{
		OPT_HELP(),
		OPT_STRING('i', "input", &input_file_path, "input file"),
		OPT_STRING('o', "output", &output_file_path, "output file"),
		OPT_BOOLEAN('l', "legal", &legal, "show legal information and quit"),
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

	if (legal)
	{
		print_legal();
		return 0;
	}

	// actual program start

	FILE* input_file = fopen(input_file_path, "rb");

	if (!input_file)
	{
		print_error("Unable to open input file");
		return 1;
	}

	int width, height, resolution;
	uint32_t input_buffer;

	int received = fread(&input_buffer, 4, 1, input_file);
	if (received != 1)
	{
		fprintf(stderr, "Invalid magic number read. Received: %i\n", received);
		goto cleanup;
	}

	if (input_buffer != I332_MAGIC_NUMBER)
	{
		print_error("Improper file type");
		goto cleanup;
	}

	if (fread(&width, 4, 1, input_file) != 1)
	{
		print_error("Invalid width read");
		goto cleanup;
	}

	if (fread(&height, 4, 1, input_file) != 1)
	{
		print_error("Invalid height read");
		goto cleanup;
	}

	if (width < 1 || height < 1)
	{
		fprintf(stderr, "Improper image resolution. Receieved: %i * %i\n", width, height);
		goto cleanup;
	}

	resolution = width * height;

	input_image = malloc(resolution);

	if (!input_image)
	{
		print_error("Unable to allocate memory for input image");
		goto cleanup;
	}

	received = fread(input_image, 1, resolution, input_file);
	if (received != resolution)
	{
		fprintf(stderr, "Invalid image data read. Expected: %i. Received: %i.\n", resolution,
		        received);
		goto cleanup;
	}

	output_image = malloc(resolution * sizeof(*output_image));

	if (!output_image)
	{
		print_error("Unable to allocate memory for output image");
		goto cleanup;
	}

	for (int i = 0; i < resolution; ++i)
	{
		uint8_t packed = input_image[i];

		uint8_t r = (packed >> 5) & r_max;
		uint8_t g = (packed >> 2) & g_max;
		uint8_t b = packed & b_max;

		output_image[i].r = (uint8_t)(((r / r_maxf) * 255.f) + 0.5f);
		output_image[i].g = (uint8_t)(((g / g_maxf) * 255.f) + 0.5f);
		output_image[i].b = (uint8_t)(((b / b_maxf) * 255.f) + 0.5f);
		output_image[i].a = 255;
	}

	ezpng_write_rgba(output_file_path, output_image, width, height);

cleanup:
	if (output_image)
		free(output_image);

	if (input_image)
		free(input_image);

	if (input_file)
		fclose(input_file);

	return 0;
}
