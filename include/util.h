#ifndef UTIL_H_
#define UTIL_H_

#include <assert.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <emmintrin.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/time.h>

#include <algorithm>
#include <cmath>

#include "libdpf/block.h"
#include "peer.h"
#include "typedef.h"

void xor_bytes(const uchar *a, const uchar *b, uint64_t length, uchar *out);
void xor_bytes(const uchar *input_a, const uchar *input_b, const uchar *input_c, uint64_t len, uchar *output);

void uint64_to_bytes(uint64_t value, uchar *bytes, uint len = 8);
uint64_t bytes_to_uint64(const uchar *b, uint len = 8);
void rand_bytes(uchar *bytes, const uint len);
uint64_t rand_uint64();
uint64_t rand_uint64(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &prg);
uint64_t timestamp();

uint64_t bit_length(uint64_t n);
uint64_t byte_length(uint64_t n);
uint64_t uint64_log2(const uint64_t n);
uint64_t uint64_ceil_divide(const uint64_t n, const uint64_t q);
uint64_t uint64_pow2_ceil(const uint64_t n);

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index = -1);

template <typename D>
D *ShareTwoThird(Peer peer[2], D &v_in_13, bool count_band) {
    peer[1].WriteData(v_in_13, count_band);
    D v_out = peer[0].template ReadData<D>(v_in_13.Size());
    return new D[2]{v_out, v_in_13};
}

template <typename D>
std::vector<D> *ShareTwoThird(Peer peer[2], std::vector<D> &v_in_13, bool count_band) {
    peer[1].WriteData(v_in_13, count_band);
    std::vector<D> v_out = peer[0].template ReadData<D>(v_in_13.size(), v_in_13[0].Size());
    return new std::vector<D>[2] { v_out, v_in_13 };
}

template <typename D>
void ShareIndexTwoThird(Peer peer[2], const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band) {
    uint64_t rand_range = n;
    peer[1].WriteLong(index_13, count_band);
    index_23[0] = peer[0].ReadLong() % rand_range;
    index_23[1] = index_13 % rand_range;
}

#endif /* UTIL_H_ */
