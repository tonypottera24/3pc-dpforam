#ifndef ZP_DATA_H_
#define ZP_DATA_H_

#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpData {
private:
    BIGNUM *data_ = NULL;
    static const inline BIGNUM *p_ = BN_get0_nist_prime_256();
    const static inline uint size_ = BN_num_bytes(BN_get0_nist_prime_256());  // 32
    static inline BN_CTX *bn_ctx_ = BN_CTX_new();
    static const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ZpData() {
        this->data_ = BN_new();
        this->Reset();
    }

    ZpData(const uint size) {
        this->data_ = BN_new();
        this->Reset();
    }

    ZpData(const ZpData &other) {
        this->data_ = BN_dup(other.data_);
    }

    ~ZpData() {
        BN_free(this->data_);
    }

    ZpData &operator=(const ZpData &other) {
        if (this == &other) return *this;
        BN_copy(this->data_, other.data_);
        return *this;
    }

    ZpData operator-() {
        BN_mod_sub(this->data_, this->p_, this->data_, this->p_, this->bn_ctx_);
        return *this;
    }

    ZpData &operator+=(const ZpData &rhs) {
        BN_mod_add(this->data_, this->data_, rhs.data_, this->p_, this->bn_ctx_);
        return *this;
    }

    ZpData &operator-=(const ZpData &rhs) {
        BN_mod_sub(this->data_, this->data_, rhs.data_, this->p_, this->bn_ctx_);
        return *this;
    }

    bool operator==(const ZpData &rhs) {
        return BN_cmp(this->data_, rhs.data_) == 0;
    }

    friend ZpData operator+(ZpData lhs, const ZpData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend ZpData operator-(ZpData lhs, const ZpData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void DumpBuffer(uchar *buffer) {
        BN_bn2binpad(this->data_, buffer, this->Size());
    }

    std::vector<uchar> DumpVector() {
        std::vector<uchar> data(this->Size());
        DumpBuffer(data.data());
        return data;
    }

    void LoadBuffer(uchar *buffer) {
        BN_bin2bn(buffer, this->Size(), this->data_);
    }

    void LoadVector(std::vector<uchar> &data) {
        LoadBuffer(data.data());
    }

    void Reset() {
        BN_zero(this->data_);
    }

    void Resize(const uint size) {}

    void Random(PRG *prg = NULL) {
        if (prg == NULL) prg = this->prg_;
        prg->RandBn(this->data_, this->p_, this->bn_ctx_);
    }

    uint Size() {
        // fprintf(stderr, "size = %u\n", this->size_);
        return this->size_;
    }

    static bool IsSymmetric() { return is_symmetric_; }

    void Print(const char *title = "") {
#ifdef DEBUG
        if (strlen(title) > 0) {
            debug_print("%s ", title);
        }
        debug_print("%s\n", BN_bn2dec(this->data_));
#endif
    }
};

#endif /* ZP_DATA_H_ */