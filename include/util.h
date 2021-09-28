#ifndef UTIL_H_
#define UTIL_H_

#include <assert.h>
#include <emmintrin.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/time.h>

#include <algorithm>
#include <cmath>

#include "libdpf/block.h"
#include "typedef.h"

void xor_bytes(const uchar *a, const uchar *b, uint length, uchar *out);
void xor_bytes(const uchar *input_a, const uchar *input_b, const uchar *input_c, uint len, uchar *output);

void uint64_to_bytes(uint64_t value, uchar *bytes);
void uint64_to_bytes(uint64_t value, uchar *bytes, uint len);
uint64_t bytes_to_uint64(const uchar *b);
uint64_t bytes_to_uint64(const uchar *b, uint len);
void rand_bytes(uchar *bytes, const uint len);
uint64_t rand_uint64();
uint64_t timestamp();

uint64_t bit_length(const uint64_t n);
uint64_t byte_length(const uint64_t n);
uint64_t uint64_log2(const uint64_t n);
uint64_t uint64_ceil_divide(const uint64_t n, const uint64_t q);

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index = -1);

#endif /* UTIL_H_ */
