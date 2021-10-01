#include "util.h"

void xor_bytes(const uchar *a, const uchar *b, uint64_t len, uchar *out) {
    for (uint64_t i = 0; i < len; i++) {
        out[i] = a[i] ^ b[i];
    }
    // for (uint64_t i = 0; i < len; i += 8) {
    //     uint64_t size = std::min(len - i, 8ULL);
    //     uint64_t *aa = (uint64_t *)&a[i];
    //     uint64_t *bb = (uint64_t *)&b[i];
    //     uint64_t o = (*aa) ^ (*bb);
    //     memcpy(&out[i], &o, size);
    // }
}

void xor_bytes(const uchar *a, const uchar *b, const uchar *c, uint64_t len, uchar *output) {
    for (uint64_t i = 0; i < len; i++) {
        output[i] = a[i] ^ b[i] ^ c[i];
    }
}

void uint64_to_bytes(uint64_t value, uchar *bytes, uint len) {
    memcpy(bytes, &value, len);
}

uint64_t bytes_to_uint64(const uchar *bytes, const uint len) {
    uint64_t value;
    memcpy(&value, bytes, len);
    return value;
}

void rand_bytes(uchar *bytes, const uint len) {
    RAND_bytes(bytes, len);
}

uint64_t rand_uint64() {
    uchar bytes[8];
    rand_bytes(bytes, 8);
    return bytes_to_uint64(bytes);
}

uint64_t rand_uint64(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption prg) {
    uchar bytes[8];
    prg.GenerateBlock(bytes, 8);
    return bytes_to_uint64(bytes);
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
    uint64_t byte_length = 0, tmp = n;
    while (tmp != 0) {
        tmp >>= 8;
        byte_length++;
    }
    return byte_length;
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