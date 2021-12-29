#ifndef EC_DATA_H_
#define EC_DATA_H_

#include <cryptopp/eccrypto.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <inttypes.h>

#include "typedef.h"
#include "util.h"

using namespace CryptoPP;

class ECData {
private:
    // const uint size_ = DL_GroupParameters_EC<ECP>(ASN1::secp256r1()).GetCurve().EncodedPointSize(false);
    ECP::Point data_;
    const DL_GroupParameters_EC<ECP> group_ = DL_GroupParameters_EC<ECP>(ASN1::secp256r1());
    const bool is_symmetric_ = false;
    const bool compressed_ = false;

public:
    ECData();
    ECData(const ECData &other);
    ECData(uchar *data, const uint size);
    ECData(const uint size, const bool set_zero = false);
    ~ECData();

    ECData &operator=(const ECData &other);
    // ECData &operator=(ECData &&other) noexcept;
    ECData operator-();
    ECData &operator+=(const ECData &rhs);
    ECData &operator-=(const ECData &rhs);
    bool operator==(const ECData &rhs);
    friend ECData operator+(ECData lhs, const ECData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend ECData operator-(ECData lhs, const ECData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    uchar *Dump();
    void Load(uchar *data, uint size);
    void ConvertFromBytes(uchar *data, uint size);
    void Reset();
    void Random(uint size);
    void Random(CTR_Mode<AES>::Encryption &prg, uint size);
    uint Size() {
        // printf("size %u\n", this->group_.GetCurve().EncodedPointSize(this->compressed_));
        // return this->group_.GetCurve().EncodedPointSize(this->compressed_);
        return 65;
    }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* EC_DATA_H_ */