#include "block.h"

#include "aes.h"
#include "libdpf.h"

static AES_KEY rand_aes_key;
static uint64_t current_rand_index;

uint128 dpf_seed(uint128 *seed) {
    uint128 cur_seed;
    current_rand_index = 0;
    if (seed) {
        cur_seed = *seed;
    } else {
        if (RAND_bytes((uchar *)&cur_seed, 16) == 0) {
            fprintf(stderr, "** unable to seed securely\n");
            return dpf_zero_block();
        }
    }
    AES_set_encrypt_key(cur_seed, &rand_aes_key);
    return cur_seed;
}

uint128 dpf_random_block(void) {
    uint128 out;
    uint64_t *val;
    int i;

    out = dpf_zero_block();
    val = (uint64_t *)&out;
    val[0] = current_rand_index++;
    out = _mm_xor_si128(out, rand_aes_key.rd_key[0]);
    for (i = 1; i < 10; ++i)
        out = _mm_aesenc_si128(out, rand_aes_key.rd_key[i]);
    return _mm_aesenclast_si128(out, rand_aes_key.rd_key[i]);
}

uint128 *dpf_allocate_blocks(size_t nblocks) {
    // int res;
    uint128 *blks = NULL;
    blks = (uint128 *)calloc(nblocks, sizeof(uint128));
    /* res = posix_memalign((void **) &blks, 128, sizeof(block) * nblocks); */
    /* if (res == 0) { */
    /*     return blks; */
    /* } else { */
    /*     perror("allocate_blocks"); */
    /*     return NULL; */
    /* } */
    return blks;
}

void _output_bit_to_bit(uint64_t input) {
    for (int i = 0; i < 64; i++) {
        if ((1ll << i) & input)
            fprintf(stderr, "1");
        else
            fprintf(stderr, "0");
    }
}

void dpf_cb(uint128 input) {
    uint64_t *val = (uint64_t *)&input;

    // fprintf(stderr,"%016lx%016lx\n", val[0], val[1]);
    _output_bit_to_bit(val[0]);
    _output_bit_to_bit(val[1]);
    fprintf(stderr, "\n");
}

// void dpf_cbnotnewline(block input)
// {
//     uint64_t *val = (uint64_t *)&input;

//     _output_bit_to_bit(val[0]);
//     _output_bit_to_bit(val[1]);
// }
