#ifndef PRG_H_
#define PRG_H_

#include <assert.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cmath>
#include <cstring>
#include <vector>

#include "libdpf/aes.h"
#include "typedef.h"

class PRG {
private:
    // uint128 seed_;
    uchar *seed_;
    // AES_KEY rand_aes_key_;
    // uint64_t current_rand_index_;
    // static const uint seed_size_ = sizeof(uint128);

    uint used_bytes_ = 0;

    BN_CTX *bn_ctx_ = BN_CTX_new();

    EVP_CIPHER_CTX *cipher_ctx_;
    uchar aes_key_[32];
    uchar aes_iv_[16];
    const static uint aes_key_size_ = 32;
    const static uint aes_iv_size_ = 16;
    const static inline EVP_CIPHER *evp_cipher_ = EVP_aes_128_ecb();
    const static inline uint seed_size_ = EVP_CIPHER_get_block_size(EVP_aes_128_ecb());

    // inline uint128 set_seed(uint128 *seed) {
    //     uint128 cur_seed;
    //     this->current_rand_index_ = 0;
    //     if (seed) {
    //         cur_seed = *seed;
    //     } else {
    //         if (RAND_bytes((uchar *)&cur_seed, 16) == 0) {
    //             fprintf(stderr, "** unable to seed securely\n");
    //             return _mm_setzero_si128();
    //         }
    //     }
    //     AES_set_encrypt_key(cur_seed, &this->rand_aes_key_);
    //     return cur_seed;
    // }

    // inline uint128 gen_random() {
    //     uint128 out;
    //     uint64_t *val;
    //     int i;

    //     out = _mm_setzero_si128();
    //     val = (uint64_t *)&out;
    //     val[0] = this->current_rand_index_++;
    //     out = _mm_xor_si128(out, this->rand_aes_key_.rd_key[0]);
    //     for (i = 1; i < 10; ++i) {
    //         out = _mm_aesenc_si128(out, this->rand_aes_key_.rd_key[i]);
    //     }
    //     return _mm_aesenclast_si128(out, this->rand_aes_key_.rd_key[i]);
    // }

public:
    PRG();
    ~PRG();
    static uint SeedSize() {
        return aes_key_size_ + aes_iv_size_ + seed_size_;
        // return seed_size_;
    }
    void SetSeed(uchar *seed);
    void RandBytes(uchar *data, uint size);
    void RandBn(BIGNUM *bn, const BIGNUM *p);
};

#endif /* PRG_H_ */