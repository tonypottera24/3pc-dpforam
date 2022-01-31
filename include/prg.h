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
    const uint seed_size_ = EVP_MD_size(EVP_sha256());
    EVP_MD_CTX *md_ctx_;
    BN_CTX *bn_ctx_;

public:
    PRG();
    ~PRG();
    void SetSeed(uchar *seed);
    void RandBytes(uchar *data, uint size);
    void RandBn(BIGNUM *bn, BIGNUM *p);
};

#endif /* PRG_H_ */