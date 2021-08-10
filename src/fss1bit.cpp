#include "fss1bit.h"

#include <string.h>
#include <x86intrin.h>

#include <algorithm>

#include "bit_perm.h"

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

// TODO: find supported cpu to test BMI2
void to_byte_vector_with_perm(uint64_t input, uchar *output, uint size,
                              uint perm) {
//#if defined(__BMI2__)
//	input = general_reverse_bits(input, perm ^ 63);
//	uchar* addr = (uchar*) &input;
//	uint64_t * data64 = (uint64_t *) output;
//	for (uint i = 0; i < size/8; ++i) {
//		uint64_t tmp = 0;
//		memcpy(&tmp, addr+i, 1);
//		data64[i] = _pdep_u64(tmp, (uint64_t) 0x0101010101010101ULL);
//	}
//#else
#pragma omp simd aligned(output, masks : 16)
    for (uint i = 0; i < size; i++) {
        output[i] = (input & masks[i ^ perm]) != 0ULL;
    }
    //#endif
}

FSS1Bit::FSS1Bit() {
    uint64_t userkey1 = 597349ULL;
    uint64_t userkey2 = 121379ULL;
    uint128 userkey = dpf_make_block(userkey1, userkey2);
    uchar seed[] = "abcdefghijklmnop";
    dpf_seed((uint128 *)seed);
    AES_set_encrypt_key(userkey, &aes_key_);
}

uint FSS1Bit::Gen(uint64_t alpha, uint log_n, uchar *keys[2]) {
    return GEN(&aes_key_, alpha, log_n, keys, keys + 1);
}

void FSS1Bit::EvalAll(const uchar *key, uint log_n, uchar *out) {
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