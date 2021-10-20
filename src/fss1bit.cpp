#include "fss1bit.h"

#include <string.h>
#include <x86intrin.h>

#include <algorithm>

const uint64_t masks[64] = {0x0000000000000001ULL, 0x0000000000000002ULL,
                            0x0000000000000004ULL, 0x0000000000000008ULL, 0x0000000000000010ULL,
                            0x0000000000000020ULL, 0x0000000000000040ULL, 0x0000000000000080ULL,
                            0x0000000000000100ULL, 0x0000000000000200ULL, 0x0000000000000400ULL,
                            0x0000000000000800ULL, 0x0000000000001000ULL, 0x0000000000002000ULL,
                            0x0000000000004000ULL, 0x0000000000008000ULL, 0x0000000000010000ULL,
                            0x0000000000020000ULL, 0x0000000000040000ULL, 0x0000000000080000ULL,
                            0x0000000000100000ULL, 0x0000000000200000ULL, 0x0000000000400000ULL,
                            0x0000000000800000ULL, 0x0000000001000000ULL, 0x0000000002000000ULL,
                            0x0000000004000000ULL, 0x0000000008000000ULL, 0x0000000010000000ULL,
                            0x0000000020000000ULL, 0x0000000040000000ULL, 0x0000000080000000ULL,
                            0x0000000100000000ULL, 0x0000000200000000ULL, 0x0000000400000000ULL,
                            0x0000000800000000ULL, 0x0000001000000000ULL, 0x0000002000000000ULL,
                            0x0000004000000000ULL, 0x0000008000000000ULL, 0x0000010000000000ULL,
                            0x0000020000000000ULL, 0x0000040000000000ULL, 0x0000080000000000ULL,
                            0x0000100000000000ULL, 0x0000200000000000ULL, 0x0000400000000000ULL,
                            0x0000800000000000ULL, 0x0001000000000000ULL, 0x0002000000000000ULL,
                            0x0004000000000000ULL, 0x0008000000000000ULL, 0x0010000000000000ULL,
                            0x0020000000000000ULL, 0x0040000000000000ULL, 0x0080000000000000ULL,
                            0x0100000000000000ULL, 0x0200000000000000ULL, 0x0400000000000000ULL,
                            0x0800000000000000ULL, 0x1000000000000000ULL, 0x2000000000000000ULL,
                            0x4000000000000000ULL, 0x8000000000000000ULL};

void to_byte_vector(uint64_t input, uchar *output, uint size) {
#pragma omp simd aligned(output, masks : 16)
    for (uint i = 0; i < size; i++) {
        output[i] = (input & masks[i]) != 0ULL;
    }
}

void to_byte_vector(uint128 input, uchar *output) {
    uint64_t *val = (uint64_t *)&input;
    to_byte_vector(val[0], output, 64);
    to_byte_vector(val[1], output + 64, 64);
}

FSS1Bit::FSS1Bit() {
    uint64_t userkey1 = 597349ULL;
    uint64_t userkey2 = 121379ULL;
    uint128 userkey = dpf_make_block(userkey1, userkey2);
    uchar seed[] = "abcdefghijklmnop";
    dpf_seed((uint128 *)seed);
    AES_set_encrypt_key(userkey, &aes_key_);
}

uint FSS1Bit::Gen(uint64_t index, uint64_t log_n, uchar *keys[2]) {
    return GEN(&aes_key_, index, log_n, keys, keys + 1);
}

void FSS1Bit::EvalAll(const uchar *key, uint64_t log_n, uchar *out) {
    uint128 *res = EVALFULL(&aes_key_, key);
    if (log_n <= 6) {
        to_byte_vector(((uint64_t *)res)[0], out, (1 << log_n));
    } else {
        uint max_layer = std::max((int)log_n - 7, 0);
        uint64_t groups = 1ULL << max_layer;
        for (uint64_t i = 0; i < groups; i++) {
            to_byte_vector(res[i], out + (i << 7));
        }
    }
    free(res);
}

void FSS1Bit::PseudoGen(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prg, uint64_t index, uint64_t byte_length, uchar *dpf_out) {
    prg->GenerateBlock(dpf_out, byte_length);
    uint64_t index_byte = index / 8ULL;
    uint64_t index_bit = index % 8ULL;
    dpf_out[index_byte] ^= 1 << index_bit;
}

void FSS1Bit::PseudoEvalAll(uchar *dpf_out, const uint64_t n, bool *dpf_out_evaluated) {
    uint64_t index_byte = 0, index_bit = 0;
    for (uint64_t i = 0; i < n; i++) {
        dpf_out_evaluated[i] = (dpf_out[index_byte] >> index_bit) & 1;
        index_bit++;
        if (index_bit == 8ULL) {
            index_byte++;
            index_bit = 0;
        }
    }
}