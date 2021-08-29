#ifndef UTIL_H_
#define UTIL_H_

#include "libdpf/block.h"
#include "typedef.h"

const __m128i masks_128[2] = {_mm_set_epi32(0, 0, 0, 0), _mm_set_epi32(
                                                             0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)};
const uchar masks_8[2] = {0x00, 0xFF};

void xor_bytes(const uchar *a, const uchar *b, uint length, uchar *out);
// void cal_xor_128(const uchar *a, const uchar *b, uint quo, uint rem, uchar *c);
inline void set_xor_128(const uchar *__restrict__ a, uint quo, uint rem,
                        uchar *__restrict__ c) {
    __m128i *aa = (__m128i *)a;
    __m128i *cc = (__m128i *)c;
    uint i;
    for (i = 0; i < quo; i++) {
        cc[i] = _mm_xor_si128(aa[i], cc[i]);
    }
    if (rem) {
        a = (uchar *)&(aa[i]);
        c = (uchar *)&(cc[i]);
#pragma omp simd
        for (i = 0; i < rem; i++) {
            c[i] ^= a[i];
        }
    }
}

inline void select_xor_128(const uchar *__restrict__ a, bool bit, uint quo, uint rem,
                           uchar *__restrict__ c) {
    __m128i *aa = (__m128i *)a;
    __m128i *cc = (__m128i *)c;
    uint i;
    for (i = 0; i < quo; i++) {
        cc[i] = _mm_xor_si128(_mm_and_si128(aa[i], masks_128[bit]), cc[i]);
    }
    if (rem) {
        a = (uchar *)&(aa[i]);
        c = (uchar *)&(cc[i]);
#pragma omp simd
        for (i = 0; i < rem; i++) {
            c[i] ^= (a[i] & masks_8[bit]);
        }
    }
}

void uint_to_bytes(uint n, uchar *b);
uint bytes_to_uint(const uchar *b);
void uint64_to_bytes(uint64_t n, uchar *b);
void uint64_to_bytes(uint64_t n, uchar *b, uint len);
uint64_t bytes_to_uint64(const uchar *b);
uint64_t bytes_to_uint64(const uchar *b, uint len);
// void rand_bytes(uchar *bytes, uint len);
void bytes_to_two_uint64(uchar *input, uint len, uint64_t output[2]);
uint64_t current_timestamp();

#endif /* UTIL_H_ */
