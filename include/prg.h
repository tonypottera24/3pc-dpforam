#ifndef PRG_H_
#define PRG_H_

#include <assert.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cmath>
#include <cstring>
#include <vector>

#include "typedef.h"

class PRG {
private:
    uchar *seed_;
    uint used_bytes_ = 0;

    static inline BN_CTX *bn_ctx_ = BN_CTX_new();
    // static inline EVP_CIPHER_CTX *cipher_ctx_ = EVP_CIPHER_CTX_new();
    // uchar aes_key_[32];
    // uchar aes_iv_[16];
    // const static uint aes_key_size_ = 32;
    // const static uint aes_iv_size_ = 16;
    // const static inline EVP_CIPHER *evp_cipher_ = EVP_aes_256_ecb();
    // const static inline uint seed_size_ = EVP_CIPHER_get_block_size(EVP_aes_256_ecb());

    static inline EVP_MD_CTX *md_ctx_ = EVP_MD_CTX_new();
    const static inline EVP_MD *evp_md_ = EVP_sha256();
    const static inline uint seed_size_ = EVP_MD_size(EVP_sha256());

public:
    PRG();
    ~PRG();
    static uint SeedSize() {
        return seed_size_;
    }
    void SetSeed(uchar *seed);
    void RandBytes(uchar *data, uint size);
    void RandBn(BIGNUM *bn, const BIGNUM *p);
};

#endif /* PRG_H_ */