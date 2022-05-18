#include "prg.h"

PRG::PRG() {
    this->seed_ = (unsigned char *)OPENSSL_malloc(this->seed_size_);
    RAND_bytes(this->seed_, this->seed_size_);

    RAND_bytes(this->aes_key_, this->aes_key_size_);
    RAND_bytes(this->aes_iv_, this->aes_iv_size_);
    this->cipher_ctx_ = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex2(this->cipher_ctx_, this->evp_cipher_, this->aes_key_, this->aes_iv_, NULL);
}

PRG::~PRG() {
    EVP_CIPHER_CTX_free(this->cipher_ctx_);
    BN_CTX_free(this->bn_ctx_);
    OPENSSL_free(this->seed_);
}

void PRG::SetSeed(uchar *seed) {
    memcpy(this->aes_key_, seed, this->aes_key_size_);
    memcpy(this->aes_iv_, seed + this->aes_key_size_, this->aes_iv_size_);
    memcpy(this->seed_, seed + this->aes_key_size_ + this->aes_iv_size_, this->seed_size_);
    EVP_EncryptInit_ex2(this->cipher_ctx_, this->evp_cipher_, this->aes_key_, this->aes_iv_, NULL);
}

void PRG::RandBytes(uchar *data, uint size) {
    // fprintf(stderr, "RandBytes, size = %u, this->used_bytes_ = %u\n", size, this->used_bytes_);

    // for (uint i = 0; i < this->seed_size_; i++) {
    //     fprintf(stderr, "%02X", this->seed_[i]);
    // }
    // fprintf(stderr, "\n");

    uint offset = 0;
    while (offset < size) {
        if (this->used_bytes_ == this->seed_size_) {
            int block_size;
            EVP_EncryptInit_ex2(this->cipher_ctx_, NULL, NULL, NULL, NULL);
            EVP_EncryptUpdate(this->cipher_ctx_, this->seed_, &block_size, this->seed_, this->seed_size_);
            EVP_EncryptFinal_ex(this->cipher_ctx_, this->seed_, &block_size);
            // fprintf(stderr, "seed_size = %u, block_size = %u\n", seed_size_, block_size);

            // fprintf(stderr, "new seed ");
            // for (uint i = 0; i < this->seed_size_; i++) {
            //     fprintf(stderr, "%02X", this->seed_[i]);
            // }
            // fprintf(stderr, "\n");

            this->used_bytes_ = 0;
        }

        uint bytes_to_copy = std::min(this->seed_size_ - this->used_bytes_, size - offset);
        memcpy(data + offset, this->seed_ + this->used_bytes_, bytes_to_copy);
        offset += bytes_to_copy;
        this->used_bytes_ += bytes_to_copy;

        // fprintf(stderr, "return data ");
        // for (uint i = 0; i < size; i++) {
        //     fprintf(stderr, "%02X", data[i]);
        // }
        // fprintf(stderr, "\n");
    }
}

void PRG::RandBn(BIGNUM *bn, const BIGNUM *p) {
    uint p_size = BN_num_bytes(p);
    uchar r[p_size];
    this->RandBytes(r, p_size);
    BN_bin2bn(r, p_size, bn);
    BN_nnmod(bn, bn, p, this->bn_ctx_);
}