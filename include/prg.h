#ifndef PRG_H_
#define PRG_H_

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <cmath>
#include <vector>

#include "typedef.h"

class PRG {
private:
    uchar *seed_;
    static inline EVP_MD_CTX *md_ctx_ = EVP_MD_CTX_new();
    static inline BN_CTX *bn_ctx_ = BN_CTX_new();

public:
    const static inline EVP_MD *evp_md_ = EVP_sha256();
    const static inline uint seed_size = EVP_MD_size(EVP_sha256());

public:
    PRG();
    ~PRG();
    void SetSeed(uchar *seed);
    void RandBytes(uchar *data, uint size);
    void RandBn(BIGNUM *bn, const BIGNUM *p);
};

#endif /* PRG_H_ */