#ifndef ZP_DATA_H_
#define ZP_DATA_H_

#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpData {
public:
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
    bool operator==(const ZpData &rhs);

    static void Add(const ZpData &a, const ZpData &b, ZpData &r);
    static void Minus(const ZpData &a, const ZpData &b, ZpData &r);

    void Dump(std::vector<uchar> &data);
    void Load(std::vector<uchar> &data);
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