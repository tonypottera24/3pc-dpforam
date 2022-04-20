#include "prg.h"

PRG::PRG() {
    this->seed_ = (unsigned char *)OPENSSL_malloc(this->seed_size_);
    RAND_bytes(this->seed_, this->seed_size_);
    // RAND_bytes(this->aes_key_, this->aes_key_size_);
    // RAND_bytes(this->aes_iv_, this->aes_iv_size_);
    EVP_DigestInit_ex2(this->md_ctx_, this->evp_md_, NULL);
}

PRG::~PRG() {
    // EVP_MD_CTX_free(this->md_ctx_);
    // BN_CTX_free(this->bn_ctx_);
    OPENSSL_free(this->seed_);
}

void PRG::SetSeed(uchar *seed) {
    // memcpy(this->aes_key_, seed, this->aes_key_size_);
    // memcpy(this->aes_iv_, seed + this->aes_key_size_, this->aes_iv_size_);
    // memcpy(this->seed_, seed + this->aes_key_size_ + this->aes_iv_size_, this->seed_size_);
    memcpy(this->seed_, seed, this->seed_size_);
}

void PRG::RandBytes(uchar *data, uint size) {
    uint offset = 0;
    while (offset < size) {
        if (this->used_bytes_ == this->seed_size_) {
            // int block_size;
            // EVP_EncryptInit_ex(this->cipher_ctx_, this->evp_cipher_, NULL, this->aes_key_, this->aes_iv_);
            // EVP_EncryptUpdate(this->cipher_ctx_, this->seed_, &block_size, this->seed_, this->seed_size_);
            // EVP_EncryptFinal_ex(this->cipher_ctx_, this->seed_, &block_size);

            uint digest_size;
            EVP_DigestInit_ex2(this->md_ctx_, NULL, NULL);
            EVP_DigestUpdate(this->md_ctx_, this->seed_, this->seed_size_);
            EVP_DigestFinal_ex(this->md_ctx_, this->seed_, &digest_size);

            this->used_bytes_ = 0;
        }

        uint bytes_to_copy = std::min(this->seed_size_ - this->used_bytes_, size - offset);
        memcpy(data + offset, this->seed_ + this->used_bytes_, bytes_to_copy);
        offset += bytes_to_copy;
        this->used_bytes_ += bytes_to_copy;
    }
}

void PRG::RandBn(BIGNUM *bn, const BIGNUM *p) {
    uint p_size = BN_num_bytes(p);
    uchar r[p_size];
    this->RandBytes(r, p_size);
    BN_bin2bn(r, p_size, bn);
    BN_mod(bn, bn, p, this->bn_ctx_);
}