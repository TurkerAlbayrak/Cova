#ifndef COVA_BASE64_H
#define COVA_BASE64_H
#include <stddef.h>

char *base64_encode(const unsigned char *data, size_t input_length);

// Base64URL encoding/decoding for JWT
char *base64url_encode(const unsigned char *data, size_t input_length);
unsigned char *base64url_decode(const char *data, size_t input_length, size_t *output_length);

#endif
