#ifndef UTIL_H_
#define UTIL_H_

#include <assert.h>
#include <emmintrin.h>
#include <string.h>
#include <sys/time.h>

#include <algorithm>
#include <cmath>
#include <thread>

#include "libdpf/block.h"
#include "peer.h"
#include "typedef.h"

void xor_bytes(const uchar *a, const uchar *b, uchar *r, const uint len);
void xor_bytes(const uchar *a, const uchar *b, const uchar *c, uchar *r, const uint len);

void uint_to_bytes(uint value, uchar *bytes, uint len);
uint bytes_to_uint(const uchar *b, uint len);
void rand_bytes(uchar *bytes, const uint len);
bool rand_bool();
uint rand_uint(PRG *prg = NULL);
uint64_t timestamp();

inline uint bit_length(uint n) {
    uint bit_length = 0;
    while (n != 0) {
        n >>= 1;
        bit_length++;
    }
    return bit_length;
}

inline uint byte_length(uint n) {
    uint byte_length = 0;
    while (n != 0) {
        n >>= 8;
        byte_length++;
    }
    return byte_length;
}

inline uint log2(const uint n) {
    assert(n > 0);
    return bit_length(n) - 1;
}

inline uint pow2_ceil(const uint n) {
    uint log_n = log2(n);
    uint clean_n = 1 << log_n;
    if (clean_n < n) {
        clean_n <<= 1;
    }
    return clean_n;
}

inline uint divide_ceil(const uint n, const uint q) {
    return (n + q - 1) / q;
}

inline bool get_buffer_bit(uchar *a, const uint i) {
    return (a[i >> 3] >> (i & 7)) & 1;
}

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index = -1);

template <typename D>
void ShareTwoThird(Peer peer[2], D &v_in_13, D v_out_23[2], bool count_band) {
    v_out_23[1] = v_in_13;
    peer[1].WriteData(v_in_13, count_band);
    peer[0].ReadData(v_out_23[0]);
}

template <typename D>
void ShareTwoThird(Peer peer[2], std::vector<D> &v_in_13, std::vector<D> v_out_23[2], bool count_band) {
    // peer[1].WriteData(v_in_13, count_band);
    // std::vector<D> v_out = peer[0].ReadData(v_in_13.size(), v_in_13[0].Size());
    v_out_23[1] = v_in_13;
    write_read_data(peer[1], v_in_13, peer[0], v_out_23[0], count_band);
}

template <typename D>
void ShareIndexTwoThird(Peer peer[2], const uint index_13, const uint n, uint index_23[2], bool count_band) {
    uint rand_range = n;
    peer[1].WriteUInt(index_13, count_band);
    index_23[0] = peer[0].ReadUInt() % rand_range;
    index_23[1] = index_13 % rand_range;
}

template <typename D>
void write_read_data(Peer &write_peer, std::vector<D> &write_data, Peer &read_peer, std::vector<D> &read_data, bool count_band) {
    assert(write_data.size() == read_data.size());
    uint size = write_data.size();
    uint data_size = write_data[0].Size();
    uint data_per_block = std::max(2048 / data_size, 1U);
    uint round = divide_ceil(size, data_per_block);
    for (uint r = 0; r < round; r++) {
        uint start_index = data_per_block * r;
        uint end_index = std::min(data_per_block * (r + 1), size);
        std::vector<D> write_buffer(write_data.begin() + start_index, write_data.begin() + end_index);
        write_peer.WriteDataVector(write_buffer, count_band);

        std::vector<D> read_buffer(end_index - start_index, D(data_size));
        read_peer.ReadDataVector(read_buffer);
        for (uint i = 0; i < read_buffer.size(); i++) {
            read_data[start_index + i] = read_buffer[i];
        }
        // read_data.insert(new_data.end(), read_buffer.begin(), read_buffer.end());
    }
}

template <typename D>
void print_array(std::vector<D> &array, const char *array_name, const int64_t array_index = -1) {
#ifdef DEBUG
    if (array_index == -1) {
        debug_print("%s:\n", array_name);
    } else {
        debug_print("%s[%llu]:\n", array_name, array_index);
    }
    for (uint i = 0; i < array.size(); i++) {
        debug_print("[%u] ", i);
        array[i].Print();
    }
    debug_print("\n");
#endif
}

#endif /* UTIL_H_ */
