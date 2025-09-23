#include "ezpng.h"
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

#define EZPNG_ERROR_NONE 0
#define EZPNG_ERROR_FILE_NOT_FOUND 1
#define EZPNG_ERROR_OPEN_WRITE_FAIL 2
#define EZPNG_ERROR_READ_STRUCT_FAIL 3
#define EZPNG_ERROR_INFO_STRUCT_FAIL 4
#define EZPNG_ERROR_PNG_READ 5
#define EZPNG_ERROR_PNG_WRITE 6
#define EZPNG_ERROR_NO_DECODER 7
#define EZPNG_ERROR_NULL_PIXELS 8
#define EZPNG_ERROR_OUT_OF_MEMORY 9

static const char* ezpng_error_msgs[] = {"No error.",
                                         "File not found.",
                                         "Failed to open file for writing.",
                                         "Failed to create PNG read struct.",
                                         "Failed to create PNG info struct.",
                                         "Error during PNG read.",
                                         "Error during PNG write.",
                                         "Decoder handle is NULL.",
                                         "Pixel buffer is NULL.",
                                         "Out of memory."};

static unsigned int current_error = EZPNG_ERROR_NONE;

const char* ezpng_get_error(void)
{
	if (current_error < (sizeof(ezpng_error_msgs) / sizeof(ezpng_error_msgs[0])))
		return ezpng_error_msgs[current_error];

	return "Unknown error.";
}

// decoder

struct ezpng_decoder
{
	int width;
	int height;
	ezpng_rgba* pixels;
};

ezpng_decoder* ezpng_decoder_open(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (!fp)
	{
		current_error = EZPNG_ERROR_FILE_NOT_FOUND;
		return NULL;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png)
	{
		fclose(fp);
		current_error = EZPNG_ERROR_READ_STRUCT_FAIL;
		return NULL;
	}

	png_infop info = png_create_info_struct(png);

	if (!info)
	{
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(fp);
		current_error = EZPNG_ERROR_INFO_STRUCT_FAIL;
		return NULL;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		current_error = EZPNG_ERROR_PNG_READ;
		return NULL;
	}

	png_init_io(png, fp);
	png_read_info(png, info);

	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_byte color = png_get_color_type(png, info);
	png_byte depth = png_get_bit_depth(png, info);

	// normalize everything to 8-bit RGBA
	if (depth == 16)
		png_set_strip_16(png);

	if (color == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	if (color == PNG_COLOR_TYPE_GRAY && depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	if (color == PNG_COLOR_TYPE_RGB || color == PNG_COLOR_TYPE_GRAY
	    || color == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	ezpng_decoder* dec = (ezpng_decoder*)malloc(sizeof(ezpng_decoder));

	if (!dec)
	{
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		current_error = EZPNG_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	dec->width = width;
	dec->height = height;
	dec->pixels = (ezpng_rgba*)malloc(sizeof(ezpng_rgba) * width * height);

	if (!dec->pixels)
	{
		free(dec);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		current_error = EZPNG_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	png_bytep* rows = (png_bytep*)malloc(sizeof(png_bytep) * height);

	if (!rows)
	{
		free(dec->pixels);
		free(dec);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(fp);
		current_error = EZPNG_ERROR_OUT_OF_MEMORY;
		return NULL;
	}

	for (int y = 0; y < height; ++y)
	{
		rows[y] = (png_bytep)(dec->pixels + (y * width));
	}

	png_read_image(png, rows);

	free(rows);
	png_destroy_read_struct(&png, &info, NULL);
	fclose(fp);

	current_error = EZPNG_ERROR_NONE;
	return dec;
}

void ezpng_decoder_close(ezpng_decoder* dec)
{
	if (dec)
	{
		free(dec->pixels);
		free(dec);
	}
}

int ezpng_decoder_get_width(const ezpng_decoder* dec)
{
	return dec ? dec->width : 0;
}

int ezpng_decoder_get_height(const ezpng_decoder* dec)
{
	return dec ? dec->height : 0;
}

const ezpng_rgba* ezpng_decoder_get_data(ezpng_decoder* dec)
{
	return dec ? dec->pixels : NULL;
}

// encoder

int ezpng_write_decoded(const char* filename, const ezpng_decoder* dec)
{
	if (!dec)
	{
		current_error = EZPNG_ERROR_NO_DECODER;
		return -1;
	}

	return ezpng_write_rgba(filename, dec->pixels, dec->width, dec->height);
}

int ezpng_write_rgba(const char* filename, const ezpng_rgba* pixels, int width, int height)
{
	if (!pixels)
	{
		current_error = EZPNG_ERROR_NULL_PIXELS;
		return -1;
	}

	FILE* fp = fopen(filename, "wb");

	if (!fp)
	{
		current_error = EZPNG_ERROR_OPEN_WRITE_FAIL;
		return -1;
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
	{
		fclose(fp);
		current_error = EZPNG_ERROR_READ_STRUCT_FAIL;
		return -1;
	}

	png_infop info = png_create_info_struct(png);

	if (!info)
	{
		png_destroy_write_struct(&png, NULL);
		fclose(fp);
		current_error = EZPNG_ERROR_INFO_STRUCT_FAIL;
		return -1;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		png_destroy_write_struct(&png, &info);
		fclose(fp);
		current_error = EZPNG_ERROR_PNG_WRITE;
		return -1;
	}

	png_init_io(png, fp);

	png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	png_bytep* rows = (png_bytep*)malloc(sizeof(png_bytep) * height);

	if (!rows)
	{
		png_destroy_write_struct(&png, &info);
		fclose(fp);
		current_error = EZPNG_ERROR_OUT_OF_MEMORY;
		return -1;
	}

	for (int y = 0; y < height; ++y)
	{
		rows[y] = (png_bytep)(pixels + (y * width));
	}

	png_write_image(png, rows);
	png_write_end(png, NULL);

	free(rows);
	png_destroy_write_struct(&png, &info);
	fclose(fp);

	current_error = EZPNG_ERROR_NONE;
	return 0;
}
