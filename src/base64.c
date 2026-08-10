#include "base64.h"
#include "memtrack.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                      'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                      'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                      'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                      'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                      '4', '5', '6', '7', '8', '9', '+', '/'};
static const int mod_table[] = {0, 2, 1};

char *base64_encode(const unsigned char *data, size_t input_length) {
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = cova_malloc(output_length + 1);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data[output_length] = '\0';
    return encoded_data;
}

char *base64url_encode(const unsigned char *data, size_t input_length) {
    char *encoded_data = base64_encode(data, input_length);
    if (!encoded_data) return NULL;
    
    // Replace + with - and / with _
    // Strip = padding
    size_t i = 0;
    while (encoded_data[i] != '\0') {
        if (encoded_data[i] == '+') encoded_data[i] = '-';
        else if (encoded_data[i] == '/') encoded_data[i] = '_';
        else if (encoded_data[i] == '=') {
            encoded_data[i] = '\0';
            break;
        }
        i++;
    }
    return encoded_data;
}

static unsigned char get_b64_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-' || c == '+') return 62;
    if (c == '_' || c == '/') return 63;
    return 0; // Or handle error
}

unsigned char *base64url_decode(const char *data, size_t input_length, size_t *output_length) {
    if (input_length == 0) return NULL;

    size_t decoded_length = (input_length * 3) / 4;
    unsigned char *decoded_data = cova_malloc(decoded_length + 1);
    if (!decoded_data) return NULL;

    size_t i = 0, j = 0;
    while (i < input_length) {
        uint32_t sextet_a = i < input_length ? get_b64_val(data[i++]) : 0;
        uint32_t sextet_b = i < input_length ? get_b64_val(data[i++]) : 0;
        uint32_t sextet_c = i < input_length ? get_b64_val(data[i++]) : 0;
        uint32_t sextet_d = i < input_length ? get_b64_val(data[i++]) : 0;

        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);

        if (j < decoded_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < decoded_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < decoded_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }
    
    // Correct length if padding was stripped
    // Since input_length doesn't contain padding, (input_length * 3) / 4 is a slight underestimate or overestimate?
    // Actually, formula for no padding: (input_length * 3) / 4. 
    // If input%4 == 2, output is exactly n bytes. 
    // Let's refine decoded_length calculation.
    size_t pad = input_length % 4;
    if (pad == 2) decoded_length = (input_length * 3) / 4;
    else if (pad == 3) decoded_length = (input_length * 3) / 4 + 1;
    else decoded_length = (input_length * 3) / 4;
    
    if (output_length) *output_length = decoded_length;
    decoded_data[decoded_length] = '\0';
    return decoded_data;
}
