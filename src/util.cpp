#include "util.h"

void xor_bytes(uchar *r, const uchar *a, const uchar *b, const uint len) {
    // for (uint64_t i = 0; i < len; i++) {
    //     out[i] = a[i] ^ b[i];
    // }
    for (uint offset = 0; offset < len; offset += sizeof(uint64_t)) {
        if (len - offset > sizeof(uint64_t)) {
            uint64_t *aa = (uint64_t *)&a[offset];
            uint64_t *bb = (uint64_t *)&b[offset];
            uint64_t o = (*aa) ^ (*bb);
            memcpy(&r[offset], &o, sizeof(uint64_t));
        } else {
            for (uint i = offset; i < len; i++) {
                r[i] = a[i] ^ b[i];
            }
        }
    }
}

void xor_bytes(uchar *r, const uchar *a, const uchar *b, const uchar *c, const uint len) {
    // for (uint64_t i = 0; i < len; i++) {
    //     output[i] = a[i] ^ b[i] ^ c[i];
    // }
    for (uint offset = 0; offset < len; offset += sizeof(uint64_t)) {
        if (len - offset > sizeof(uint64_t)) {
            uint64_t *aa = (uint64_t *)&a[offset];
            uint64_t *bb = (uint64_t *)&b[offset];
            uint64_t *cc = (uint64_t *)&c[offset];
            uint64_t o = (*aa) ^ (*bb) ^ (*cc);
            memcpy(&r[offset], &o, sizeof(uint64_t));
        } else {
            for (uint i = offset; i < len; i++) {
                r[i] = a[i] ^ b[i] ^ c[i];
            }
        }
    }
}

void uint_to_bytes(uint value, uchar *bytes, uint len) {
    // TODO check small endiens / large endiens... if small, some bytes may be truncated
    memcpy(bytes, &value, len);
}

uint bytes_to_uint(const uchar *bytes, const uint len) {
    uint value = 0;
    memcpy(&value, bytes, len);
    return value;
}

void rand_bytes(uchar *bytes, const uint size) {
    RAND_bytes(bytes, size);
}

bool rand_bool() {
    uchar value;
    rand_bytes(&value, sizeof(uchar));
    return value & 1;
}

uint rand_uint(PRG *prg) {
    uint value;
    if (prg == NULL) {
        rand_bytes((uchar *)&value, sizeof(uint));
    } else {
        prg->RandBytes((uchar *)&value, sizeof(uint));
    }
    return value;
}

uint64_t timestamp() {
    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    // return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
    using namespace std::chrono;
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

uint bit_length(uint n) {
    uint bit_length = 0;
    while (n != 0) {
        n >>= 1;
        bit_length++;
    }
    return bit_length;
}

uint byte_length(uint n) {
    uint byte_length = 0;
    while (n != 0) {
        n >>= 8;
        byte_length++;
    }
    return byte_length;
}

uint log2(const uint n) {
    assert((n > 0) && "uint64_log2 n <= 0");
    return bit_length(n) - 1;
}

uint pow2_ceil(const uint n) {
    uint log_n = log2(n);
    uint clean_n = 1 << log_n;
    if (clean_n < n) {
        clean_n <<= 1;
    }
    return clean_n;
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