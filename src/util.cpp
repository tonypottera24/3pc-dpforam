#include "util.h"

void xor_bytes(const uchar *a, const uchar *b, uchar *r, const uint len) {
    uint offset = 0;
    // for (uint i = offset; i + sizeof(uint512) <= len; i += sizeof(uint512)) {
    //     uint512 *aa = (uint512 *)&a[offset];
    //     uint512 *bb = (uint512 *)&b[offset];
    //     uint512 *rr = (uint512 *)&r[offset];
    //     (*rr) = _mm512_xor_epi64(*aa, *bb);
    //     offset = i + sizeof(uint512);
    // }
    // for (uint i = offset; i + sizeof(uint256) <= len; i += sizeof(uint256)) {
    //     uint256 *aa = (uint256 *)&a[offset];
    //     uint256 *bb = (uint256 *)&b[offset];
    //     uint256 *rr = (uint256 *)&r[offset];
    //     // (*rr) = _mm256_xor_epi64(*aa, *bb);
    //     (*rr) = _mm256_xor_si256(*aa, *bb);
    //     offset = i + sizeof(uint256);
    // }
    for (uint i = offset; i + sizeof(uint128) <= len; i += sizeof(uint128)) {
        uint128 *aa = (uint128 *)&a[offset];
        uint128 *bb = (uint128 *)&b[offset];
        uint128 *rr = (uint128 *)&r[offset];
        (*rr) = _mm_xor_si128(*aa, *bb);
        offset = i + sizeof(uint128);
    }
    for (uint i = offset; i + sizeof(uint64_t) <= len; i += sizeof(uint64_t)) {
        uint64_t *aa = (uint64_t *)&a[offset];
        uint64_t *bb = (uint64_t *)&b[offset];
        uint64_t *rr = (uint64_t *)&r[offset];
        (*rr) = (*aa) ^ (*bb);
        offset = i + sizeof(uint64_t);
    }
    for (uint i = offset; i + sizeof(uint32_t) <= len; i += sizeof(uint32_t)) {
        uint32_t *aa = (uint32_t *)&a[offset];
        uint32_t *bb = (uint32_t *)&b[offset];
        uint32_t *rr = (uint32_t *)&r[offset];
        (*rr) = (*aa) ^ (*bb);
        offset = i + sizeof(uint32_t);
    }
    for (uint i = offset; i + sizeof(uint16_t) <= len; i += sizeof(uint16_t)) {
        uint16_t *aa = (uint16_t *)&a[offset];
        uint16_t *bb = (uint16_t *)&b[offset];
        uint16_t *rr = (uint16_t *)&r[offset];
        (*rr) = (*aa) ^ (*bb);
        offset = i + sizeof(uint16_t);
    }
    for (uint i = offset; i < len; i++) {
        r[i] = a[i] ^ b[i];
    }
}

void xor_bytes(const uchar *a, const uchar *b, const uchar *c, uchar *r, const uint len) {
    uint offset = 0;
    // for (uint i = offset; i + sizeof(uint512) <= len; i += sizeof(uint512)) {
    //     uint512 *aa = (uint512 *)&a[offset];
    //     uint512 *bb = (uint512 *)&b[offset];
    //     uint512 *cc = (uint512 *)&c[offset];
    //     uint512 *rr = (uint512 *)&r[offset];
    //     (*rr) = (*aa) ^ (*bb) ^ (*cc);
    //     offset = i + sizeof(uint512);
    // }
    // for (uint i = offset; i + sizeof(uint256) <= len; i += sizeof(uint256)) {
    //     uint256 *aa = (uint256 *)&a[offset];
    //     uint256 *bb = (uint256 *)&b[offset];
    //     uint256 *cc = (uint256 *)&c[offset];
    //     uint256 *rr = (uint256 *)&r[offset];
    //     (*rr) = (*aa) ^ (*bb) ^ (*cc);
    //     offset = i + sizeof(uint256);
    // }
    for (uint i = offset; i + sizeof(uint128) <= len; i += sizeof(uint128)) {
        uint128 *aa = (uint128 *)&a[offset];
        uint128 *bb = (uint128 *)&b[offset];
        uint128 *cc = (uint128 *)&c[offset];
        uint128 *rr = (uint128 *)&r[offset];
        (*rr) = (*aa) ^ (*bb) ^ (*cc);
        offset = i + sizeof(uint128);
    }
    for (uint i = offset; i + sizeof(uint64_t) <= len; i += sizeof(uint64_t)) {
        uint64_t *aa = (uint64_t *)&a[offset];
        uint64_t *bb = (uint64_t *)&b[offset];
        uint64_t *cc = (uint64_t *)&c[offset];
        uint64_t *rr = (uint64_t *)&r[offset];
        (*rr) = (*aa) ^ (*bb) ^ (*cc);
        offset = i + sizeof(uint64_t);
    }
    for (uint i = offset; i + sizeof(uint32_t) <= len; i += sizeof(uint32_t)) {
        uint32_t *aa = (uint32_t *)&a[offset];
        uint32_t *bb = (uint32_t *)&b[offset];
        uint32_t *cc = (uint32_t *)&c[offset];
        uint32_t *rr = (uint32_t *)&r[offset];
        (*rr) = (*aa) ^ (*bb) ^ (*cc);
        offset = i + sizeof(uint32_t);
    }
    for (uint i = offset; i + sizeof(uint16_t) <= len; i += sizeof(uint16_t)) {
        uint16_t *aa = (uint16_t *)&a[offset];
        uint16_t *bb = (uint16_t *)&b[offset];
        uint16_t *cc = (uint16_t *)&c[offset];
        uint16_t *rr = (uint16_t *)&r[offset];
        (*rr) = (*aa) ^ (*bb) ^ (*cc);
        offset = i + sizeof(uint16_t);
    }
    for (uint i = offset; i < len; i++) {
        r[i] = a[i] ^ b[i] ^ c[i];
    }
}

void neg_bytes(const uchar *a, uchar *r, const uint len) {
    uint offset = 0;
    for (uint i = offset; i + sizeof(uint128) <= len; i += sizeof(uint128)) {
        uint128 *aa = (uint128 *)&a[offset];
        uint128 *rr = (uint128 *)&r[offset];
        (*rr) = ~(*aa);
        offset = i + sizeof(uint128);
    }
    for (uint i = offset; i + sizeof(uint64_t) <= len; i += sizeof(uint64_t)) {
        uint64_t *aa = (uint64_t *)&a[offset];
        uint64_t *rr = (uint64_t *)&r[offset];
        (*rr) = ~(*aa);
        offset = i + sizeof(uint64_t);
    }
    for (uint i = offset; i + sizeof(uint32_t) <= len; i += sizeof(uint32_t)) {
        uint32_t *aa = (uint32_t *)&a[offset];
        uint32_t *rr = (uint32_t *)&r[offset];
        (*rr) = ~(*aa);
        offset = i + sizeof(uint32_t);
    }
    for (uint i = offset; i + sizeof(uint16_t) <= len; i += sizeof(uint16_t)) {
        uint16_t *aa = (uint16_t *)&a[offset];
        uint16_t *rr = (uint16_t *)&r[offset];
        (*rr) = ~(*aa);
        offset = i + sizeof(uint16_t);
    }
    for (uint i = offset; i < len; i++) {
        r[i] = ~a[i];
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
    assert(n > 0);
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

uint divide_ceil(const uint n, const uint q) {
    return (n + q - 1) / q;
}

bool get_buffer_bit(uchar *a, const uint i) {
    return (a[i >> 3] >> (i & 7)) & 1;
}

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index) {
#ifdef DEBUG
    if (array_index == -1) {
        debug_print("%s: 0x", array_name);
    } else {
        debug_print("%s[%ld]: 0x", array_name, array_index);
    }
    for (uint i = 0; i < len; i++) {
        debug_print("%02X", bytes[i]);
    }
    debug_print("\n");
#endif
}