#include "util.h"

void xor_bytes(uchar *r, const uchar *a, const uchar *b, const uint len) {
    // for (uint64_t i = 0; i < len; i++) {
    //     out[i] = a[i] ^ b[i];
    // }
    for (uint offset = 0; offset < len; offset += sizeof(uint64_t)) {
        if (len - offset > sizeof(uint64_t)) {
            uint64_t *aa = (uint64_t *)&a[offset];
            uint64_t *bb = (uint64_t *)&b[offset];
            uint64_t *rr = (uint64_t *)&r[offset];
            (*rr) = (*aa) ^ (*bb);
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
            uint64_t *rr = (uint64_t *)&r[offset];
            (*rr) = (*aa) ^ (*bb) ^ (*cc);
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

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index) {
#ifdef DEBUG
    if (array_index == -1) {
        debug_print("%s: 0x", array_name);
    } else {
        debug_print("%s[%llu]: 0x", array_name, array_index);
    }
    for (uint i = 0; i < len; i++) {
        debug_print("%02X", bytes[i]);
    }
    debug_print("\n");
#endif
}