#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "legal.h"

const int r_max = (1 << 3) - 1, g_max = (1 << 3) - 1, b_max = (1 << 2) - 1;
const float r_maxf = (float)r_max, g_maxf = (float)g_max, b_maxf = (float)b_max;

extern const uint32_t I332_MAGIC_NUMBER;

typedef struct RGBF
{
	float r, g, b;
} RGBF;

static void print_legal()
{
	for (int i = 0; i < LEGAL_TXT_SIZE; ++i)
	{
		puts(legal_txt[i]);
	}
}

#ifdef __cplusplus
}
#endif

#endif
