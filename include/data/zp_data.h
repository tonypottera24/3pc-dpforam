#ifndef ZP_DATA_H_
#define ZP_DATA_H_

#include <inttypes.h>

#include "benchmark/constant.h"
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
    ZpData();
    ZpData(const uint size);
    ZpData(const ZpData &other);
    ~ZpData();

    ZpData &operator=(const ZpData &other);
    ZpData &operator+=(const ZpData &rhs);
    ZpData &operator-=(const ZpData &rhs);
    bool operator==(const ZpData &rhs);

    ZpData operator-();
    friend ZpData operator+(ZpData lhs, const ZpData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend ZpData operator-(ZpData lhs, const ZpData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void DumpBuffer(uchar *buffer);
    std::vector<uchar> DumpVector();
    void LoadBuffer(uchar *buffer);

    uint64_t hash(uint64_t digest_n, uint round);

    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);

    uint Size() { return this->size_; }
    static bool IsSymmetric() { return is_symmetric_; }

    void Print(const char *title = "");
};

#endif /* ZP_DATA_H_ */