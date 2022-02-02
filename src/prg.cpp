#include "prg.h"

PRG::PRG() {
    this->seed_ = (unsigned char *)OPENSSL_malloc(this->seed_size);
    RAND_bytes(this->seed_, this->seed_size);
}

PRG::~PRG() {
    // EVP_MD_CTX_free(this->md_ctx_);
    // BN_CTX_free(this->bn_ctx_);
    free(this->seed_);
}

void PRG::SetSeed(uchar *seed) {
    memcpy(this->seed_, seed, this->seed_size);
}

void PRG::RandBytes(uchar *data, uint size) {
    uint offset = 0;
    while (offset < size) {
        EVP_DigestInit_ex(this->md_ctx_, this->hash, NULL);
        EVP_DigestUpdate(this->md_ctx_, this->seed_, seed_size);
        uint digest_size;
        EVP_DigestFinal_ex(this->md_ctx_, this->seed_, &digest_size);
        if (offset + digest_size > size) {
            digest_size = size - offset;
        }
        memcpy(&data[offset], this->seed_, digest_size);
        offset += digest_size;
    }
}

void PRG::RandBn(BIGNUM *bn, const BIGNUM *p) {
    uint p_size = BN_num_bytes(p);
    uchar r[p_size];
    this->RandBytes(r, p_size);
    BN_bin2bn(r, p_size, bn);
    BN_mod(bn, bn, p, this->bn_ctx_);
}