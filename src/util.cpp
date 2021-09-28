#include "util.h"

void xor_bytes(const uchar *a, const uchar *b, uint len, uchar *output) {
    for (uint i = 0; i < len; i++) {
        output[i] = a[i] ^ b[i];
    }
}

void xor_bytes(const uchar *a, const uchar *b, const uchar *c, uint len, uchar *output) {
    for (uint i = 0; i < len; i++) {
        output[i] = a[i] ^ b[i] ^ c[i];
    }
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

uint64_t bytes_to_uint64(const uchar *bytes, const uint len) {
    uint64_t value = 0;
    uint min = std::min(len, 8u);
    for (uint i = 0; i < min; i++) {
        value <<= 8;
        value |= bytes[len - min + i];
    }
    return value;
}

void rand_bytes(uchar *bytes, const uint len) {
    RAND_bytes(bytes, len);
}

uint64_t rand_uint64() {
    uchar bytes[8];
    rand_bytes(bytes, 8);
    uint64_t value = bytes_to_uint64(bytes);
    return value;
}

uint64_t timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
}

uint64_t bit_length(const uint64_t n) {
    uint64_t bit_length = 0, tmp = n;
    while (tmp != 0) {
        tmp >>= 1;
        bit_length++;
    }
    return bit_length;
}

uint64_t byte_length(const uint64_t n) {
    uint64_t byte_length = (bit_length(n) + 7ULL) / 8ULL;
    return byte_length == 0 ? 1 : byte_length;
}

uint64_t uint64_log2(const uint64_t n) {
    assert((n > 0) && "uint64_log2 n <= 0");
    return bit_length(n) - 1;
}

uint64_t uint64_ceil_divide(const uint64_t n, const uint64_t q) {
    return (n + q - 1ULL) / q;
}

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index) {
    if (array_index == -1) {
        printf("%s: 0x", array_name);
    } else {
        printf("%s[%llu]: 0x", array_name, array_index);
    }
    for (uint i = 0; i < len; i++) {
        printf("%02X", bytes[i]);
    }
    printf("\n");
}