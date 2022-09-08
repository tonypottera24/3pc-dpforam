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
    // TODO use AVX aes
private:
    uchar *seed_;
    uint used_bytes_ = 0;
    BN_CTX *bn_ctx_ = BN_CTX_new();
    EVP_CIPHER_CTX *cipher_ctx_;
    uchar aes_key_[32];
    uchar aes_iv_[16];
    const static uint aes_key_size_ = 32;
    const static uint aes_iv_size_ = 16;
    const static inline EVP_CIPHER *evp_cipher_ = EVP_aes_128_cbc();
    const static inline uint seed_size_ = EVP_CIPHER_get_block_size(EVP_aes_128_cbc());

public:
    PRG();
    ~PRG();
    static uint SeedSize() {
        return aes_key_size_ + aes_iv_size_ + seed_size_;
    }
    void SetSeed(uchar *seed);
    void RandBytes(uchar *data, uint size);
    void RandBn(BIGNUM *bn, const BIGNUM *p, BN_CTX *bn_ctx);
};

#endif /* PRG_H_ */