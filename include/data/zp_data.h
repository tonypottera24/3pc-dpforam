#ifndef ZP_DATA_H_
#define ZP_DATA_H_

#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpData {
private:
    BIGNUM *data_;
    static const inline BIGNUM *p_ = BN_get0_nist_prime_256();
    // const static inline uint size_ = 64;
    static inline BN_CTX *bn_ctx_ = BN_CTX_new();
    const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ZpData();
    ZpData(const ZpData &other);
    ZpData(const uint size);
    ~ZpData();

    ZpData &operator=(const ZpData &other);
    ZpData operator-();
    ZpData &operator+=(const ZpData &rhs);
    ZpData &operator-=(const ZpData &rhs);
    bool operator==(const ZpData &rhs);
    friend ZpData operator+(ZpData lhs, const ZpData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend ZpData operator-(ZpData lhs, const ZpData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    std::vector<uchar> Dump();
    void Load(std::vector<uchar> data);
    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);
    uint Size() {
        return BN_num_bytes(this->p_);
        // return this->size_;
    }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* ZP_DATA_H_ */