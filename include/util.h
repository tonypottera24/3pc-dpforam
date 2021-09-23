#ifndef UTIL_H_
#define UTIL_H_

#include "libdpf/block.h"
#include "typedef.h"

const __m128i masks_128[2] = {_mm_set_epi32(0, 0, 0, 0), _mm_set_epi32(
                                                             0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF)};
const uchar masks_8[2] = {0x00, 0xFF};

void xor_bytes(const uchar *a, const uchar *b, uint length, uchar *out);
void xor_bytes(const uchar *input_a, const uchar *input_b, const uchar *input_c, uint len, uchar *output);

void uint64_to_bytes(uint64_t value, uchar *bytes);
void uint64_to_bytes(uint64_t value, uchar *bytes, uint len);
uint64_t bytes_to_uint64(const uchar *b);
uint64_t bytes_to_uint64(const uchar *b, uint len);
void bytes_to_bytes_array(uchar *input, const uint input_size, const uint output_size, uchar **output);
void bytes_array_to_bytes(uchar **input, const uint input_size, const uint bytes_per_block, uchar *output);
void rand_bytes(uchar *bytes, const uint len);
uint64_t rand_uint64();
uint64_t timestamp();
void print_bytes(uchar *bytes, uint len, char *description);

#endif /* UTIL_H_ */
