#include "util.h"

#include <assert.h>
#include <emmintrin.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/time.h>

#include <algorithm>

void xor_bytes(const uchar *input_a, const uchar *input_b, uint len, uchar *output) {
    for (uint i = 0; i < len; i++) {
        output[i] = input_a[i] ^ input_b[i];
    }
}

void uint_to_bytes(uint value, uchar *bytes) {
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
}

uint bytes_to_uint(const uchar *bytes) {
    uint value = 0;
    for (uint i = 0; i < 4; i++) {
        value <<= 8;
        value |= bytes[i];
    }
    return value;
}

void uint64_to_bytes(uint64_t value, uchar *bytes) {
    bytes[0] = (value >> 56) & 0xFF;
    bytes[1] = (value >> 48) & 0xFF;
    bytes[2] = (value >> 40) & 0xFF;
    bytes[3] = (value >> 32) & 0xFF;
    bytes[4] = (value >> 24) & 0xFF;
    bytes[5] = (value >> 16) & 0xFF;
    bytes[6] = (value >> 8) & 0xFF;
    bytes[7] = value & 0xFF;
}

void uint64_to_bytes(uint64_t value, uchar *bytes, uint len) {
    for (uint i = 0; i < std::min(len, 8u); i++) {
        bytes[len - 1 - i] = value & 0xFF;
        value >>= 8;
    }
    if (len > 8) {
        memset(bytes, 0, len - 8);
    }
}

uint64_t bytes_to_uint64(const uchar *bytes) {
    uint64_t value = 0;
    for (uint i = 0; i < 8; i++) {
        value <<= 8;
        value |= bytes[i];
    }
    return value;
}

uint64_t bytes_to_uint64(const uchar *bytes, uint len) {
    uint64_t value = 0;
    uint min = std::min(len, 8u);
    for (uint i = 0; i < min; i++) {
        value <<= 8;
        value |= bytes[len - min + i];
    }
    return value;
}

uint64_t current_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
}

void bytes_to_bytes_array(const uchar *input, const uint input_size, const uint output_size, uchar **output) {
    uint bytes_per_block = (input_size + output_size - 1) / output_size;
    for (uint i = 0; i < output_size; i++) {
        output[i] = new uchar[bytes_per_block];
        memcpy(output[i], &input[i * bytes_per_block], bytes_per_block);
    }
}