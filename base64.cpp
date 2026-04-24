#include "stdafx.h"

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "base64.h"

#define BAD     -1
#define DECODE64(c)  (isascii(c) ? base64val[c] : BAD)

static const char base64digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char base64val[128];

void encode_base64(unsigned char *out, unsigned char *in, int inlen)
{
	for (; inlen >= 3; inlen -= 3)											//3바이트를 6비트 단위로 끊어서 4바이트로 만듬.
	{
		*out++ = base64digits[in[0] >> 2];									//0번째 앞6자리
		*out++ = base64digits[((in[0] << 4) & 0x30) | (in[1] >> 4)];		//0번째 뒤2자리 | 1번째 앞4자리
		*out++ = base64digits[((in[1] << 2) & 0x3c) | (in[2] >> 6)];		//1번째 뒤4자리 | 2번째 앞2자리
		*out++ = base64digits[in[2] & 0x3f];								//2번째 뒤6자리
		in += 3;
	}

	if (inlen > 0)
	{
		unsigned char fragment;

		*out++ = base64digits[in[0] >> 2];
		fragment = (in[0] << 4) & 0x30;

		if (inlen > 1)  fragment |= in[1] >> 4;

		*out++ = base64digits[fragment];
		*out++ = (inlen < 2) ? '=' : base64digits[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}
	*out = '\0';
}

char* decode_base64(char encoded[], int len_str)
{
	if (len_str == 0) return NULL;

	char* decoded_string;
	decoded_string = (char*)malloc(sizeof(char) * len_str);

	int i, j, k = 0;
	int num = 0;

	int count_bits = 0;

	for (i = 0; i < len_str; i += 4)
	{
		num = 0, count_bits = 0;
		for (j = 0; j < 4; j++)
		{
			if (encoded[i + j] != '=')
			{
				num = num << 6;
				count_bits += 6;
			}
			if (encoded[i + j] >= 'A' && encoded[i + j] <= 'Z')
				num = num | (encoded[i + j] - 'A');
			else if (encoded[i + j] >= 'a' && encoded[i + j] <= 'z')
				num = num | (encoded[i + j] - 'a' + 26);
			else if (encoded[i + j] >= '0' && encoded[i + j] <= '9')
				num = num | (encoded[i + j] - '0' + 52);
			else if (encoded[i + j] == '+')
				num = num | 62;
			else if (encoded[i + j] == '/')
				num = num | 63;
			else
			{
				num = num >> 2;
				count_bits -= 2;
			}
		}
		while (count_bits != 0)
		{
			count_bits -= 8;
			decoded_string[k++] = (num >> count_bits) & 255;
		}
	}
	decoded_string[k] = '\0';
	return decoded_string;
}



