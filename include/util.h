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

void xor_bytes(uchar *r, const uchar *a, const uchar *b, const uint len);
void xor_bytes(uchar *r, const uchar *a, const uchar *b, const uchar *c, const uint len);

void uint_to_bytes(uint value, uchar *bytes, uint len);
uint bytes_to_uint(const uchar *b, uint len);
void rand_bytes(uchar *bytes, const uint len);
bool rand_bool();
uint rand_uint(PRG *prg = NULL);
uint64_t timestamp();

uint bit_length(uint n);
uint byte_length(uint n);
uint log2(const uint n);
inline uint divide_ceil(const uint n, const uint q) {
    return (n + q - 1) / q;
}
uint pow2_ceil(const uint n);

inline bool get_buffer_bit(uchar *a, const uint i) {
    return (a[i >> 3] >> (i & 7)) & 1;
}

void print_bytes(const uchar *bytes, const uint len, const char *array_name, const int64_t array_index = -1);

template <typename D>
std::vector<D> ShareTwoThird(Peer peer[2], D &v_in_13, bool count_band) {
    peer[1].WriteData(v_in_13, count_band);
    D v_out = peer[0].template ReadData<D>(v_in_13.Size());
    return {v_out, v_in_13};
}

template <typename D>
std::vector<std::vector<D>> ShareTwoThird(Peer peer[2], std::vector<D> &v_in_13, bool count_band) {
    // peer[1].WriteData(v_in_13, count_band);
    // std::vector<D> v_out = peer[0].template ReadData<D>(v_in_13.size(), v_in_13[0].Size());
    std::vector<D> v_out = write_read_data(peer[1], v_in_13, peer[0], v_in_13.size(), v_in_13[0].Size(), count_band);
    return {v_out, v_in_13};
}

template <typename D>
void ShareIndexTwoThird(Peer peer[2], const uint index_13, const uint n, uint index_23[2], bool count_band) {
    uint rand_range = n;
    peer[1].WriteUInt(index_13, count_band);
    index_23[0] = peer[0].ReadUInt() % rand_range;
    index_23[1] = index_13 % rand_range;
}

template <typename D>
std::vector<D> write_read_data(Peer &write_peer, std::vector<D> &data, Peer &read_peer, const uint size, const uint data_size, bool count_band) {
    std::vector<D> new_data;
    uint data_per_block = std::max(1024 / data_size, 1U);
    uint round = divide_ceil(size, data_per_block);
    for (uint r = 0; r < round; r++) {
        std::vector<D> tmp_write_data;
        uint start_index = data_per_block * r;
        uint end_index = std::min(data_per_block * (r + 1), size);
        tmp_write_data.insert(tmp_write_data.begin(), data.begin() + start_index, data.begin() + end_index);
        write_peer.WriteData(tmp_write_data, count_band);

        std::vector<D> tmp_read_data = read_peer.ReadData<D>(end_index - start_index, data_size);
        new_data.insert(new_data.end(), tmp_read_data.begin(), tmp_read_data.end());
    }
    return new_data;
}

template <typename D>
void print_array(std::vector<D> &array, const char *array_name, const int64_t array_index = -1) {
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
}

#endif /* UTIL_H_ */
