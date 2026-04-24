#pragma once

void encode_base64(unsigned char *out, unsigned char *in, int inlen);
char* decode_base64(char encoded[], int len_str);
