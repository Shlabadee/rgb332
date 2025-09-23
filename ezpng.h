#ifndef EZPNG_H
#define EZPNG_H

#include <stdint.h>

/*
Introducing the world's laziest PNG library
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ezpng_rgba
{
	uint8_t r, g, b, a;
} ezpng_rgba;

typedef struct ezpng_decoder ezpng_decoder;
ezpng_decoder* ezpng_decoder_open(const char* filename);
void ezpng_decoder_close(ezpng_decoder* dec);
int ezpng_decoder_get_width(const ezpng_decoder* dec);
int ezpng_decoder_get_height(const ezpng_decoder* dec);
const ezpng_rgba* ezpng_decoder_get_data(ezpng_decoder* dec);

typedef struct ezpng_encoder ezpng_encoder;
int ezpng_write_decoded(const char* filename, const ezpng_decoder* dec);
int ezpng_write_rgba(const char* filename, const ezpng_rgba* pixels, int width, int height);
const char* ezpng_get_error(void);

#ifdef __cplusplus
}
#endif

#endif /* EZPNG_H */
