#ifndef LIBDPF_BLOCK_H
#define LIBDPF_BLOCK_H

#include <emmintrin.h>
#include <immintrin.h>
#include <openssl/rand.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <wmmintrin.h>
#include <xmmintrin.h>

typedef __m128i uint128;
typedef __m256i uint256;
// typedef __m512i uint512;

#define uint128_xor(x, y) _mm_xor_si128(x, y)
#define dpf_zero_block() _mm_setzero_si128()
#define dpf_equal(x, y) (_mm_movemask_epi8(_mm_cmpeq_epi8(x, y)) == 0xffff)
#define dpf_unequal(x, y) (_mm_movemask_epi8(_mm_cmpeq_epi8(x, y)) != 0xffff)

#define dpf_lsb(x) (*((char *)&x) & 1)
#define dpf_make_block(X, Y) _mm_set_epi64((__m64)(X), (__m64)(Y))
#define dpf_double(B) _mm_slli_epi64(B, 1)

#define dpf_left_shirt(v, n)                       \
    (                                              \
        {                                          \
            __m128i v1, v2;                        \
                                                   \
            if ((n) >= 64) {                       \
                v1 = _mm_slli_si128(v, 8);         \
                v1 = _mm_slli_epi64(v1, (n)-64);   \
            } else {                               \
                v1 = _mm_slli_epi64(v, n);         \
                v2 = _mm_slli_si128(v, 8);         \
                v2 = _mm_srli_epi64(v2, 64 - (n)); \
                v1 = _mm_or_si128(v1, v2);         \
            }                                      \
            v1;                                    \
        })

uint128 dpf_seed(uint128 *seed);
uint128 dpf_random_block();
uint128 *dpf_allocate_blocks(size_t nblocks);

void dpf_cb(uint128 input);
// void dpf_cbnotnewline(block input);

#endif
