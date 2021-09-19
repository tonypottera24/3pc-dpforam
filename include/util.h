#ifndef UTIL_H_
#define UTIL_H_

#include "libdpf/block.h"
#include "typedef.h"

const __m128i masks_128[2] = {_mm_set_epi32(0, 0, 0, 0), _mm_set_epi32(
                                                             0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)};
const uchar masks_8[2] = {0x00, 0xFF};

void xor_bytes(const uchar *a, const uchar *b, uint length, uchar *out);

void uint_to_bytes(uint n, uchar *b);
uint bytes_to_uint(const uchar *b);
void uint64_to_bytes(uint64_t n, uchar *b);
void uint64_to_bytes(uint64_t n, uchar *b, uint len);
uint64_t bytes_to_uint64(const uchar *b);
uint64_t bytes_to_uint64(const uchar *b, uint len);
void bytes_to_bytes_array(const uchar *input, const uint input_size, const uint output_size, uchar **output);
void bytes_array_to_bytes(uchar **input, const uint input_size, const uint bytes_per_block, uchar *output);
uint64_t rand_uint64(int64_t range);
uint64_t current_timestamp();

#endif /* UTIL_H_ */
