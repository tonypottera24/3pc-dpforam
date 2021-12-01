#ifndef ZP_DATA_H_
#define ZP_DATA_H_

#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpData {
private:
    const uint size_ = sizeof(uint64_t);
    uint64_t data_;
    const uint64_t p_ = 11;
    const uint64_t q_ = p_ - 1;
    const bool is_symmetric_ = false;

public:
    ZpData();
    ZpData(const ZpData &other);
    ZpData(uchar *data, const uint size);
    ZpData(const uint size, const bool set_zero = false);
    ~ZpData();

    ZpData &operator=(const ZpData &other);
    // ZpData &operator=(ZpData &&other) noexcept;
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

    uchar *Dump();
    void Load(uchar *data, uint size);
    void ConvertFromBytes(uchar *data, uint size);
    void Reset();
    void Random(uint size);
    void Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &prg, uint size);
    uint Size() { return this->size_; }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* ZP_DATA_H_ */