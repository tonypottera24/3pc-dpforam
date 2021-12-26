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

void xor_bytes(const uchar *a, const uchar *b, uint len, uchar *out);
void xor_bytes(const uchar *input_a, const uchar *input_b, const uchar *input_c, uint len, uchar *output);

void uint_to_bytes(uint value, uchar *bytes, uint len);
uint bytes_to_uint(const uchar *b, uint len);
void rand_bytes(uchar *bytes, const uint len);
uint rand_uint();
uint rand_uint(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &prg);
uint64_t timestamp();

uint bit_length(uint n);
uint byte_length(uint n);
uint log2(const uint n);
inline uint divide_ceil(const uint n, const uint q) {
    return (n + q - 1) / q;
}
uint pow2_ceil(const uint n);

inline bool getbit(uchar *a, const uint i) {
    return (a[i >> 8] >> (i & 127)) & 1;
}

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
void ShareIndexTwoThird(Peer peer[2], const uint index_13, const uint n, uint index_23[2], bool count_band) {
    uint rand_range = n;
    peer[1].WriteUInt(index_13, count_band);
    index_23[0] = peer[0].ReadUInt() % rand_range;
    index_23[1] = index_13 % rand_range;
}

#endif /* UTIL_H_ */
