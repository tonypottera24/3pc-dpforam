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
    ECP::Point data_;
    const bool compressed_ = false;
    static inline const DL_GroupParameters_EC<ECP> group_ = DL_GroupParameters_EC<ECP>(ASN1::secp256r1());
    static inline const uint size_ = DL_GroupParameters_EC<ECP>(ASN1::secp256r1()).GetCurve().EncodedPointSize(false);
    const bool is_symmetric_ = false;
    static inline AutoSeededRandomPool prg_;
    static inline const Integer p_ = Integer("0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff");
    static inline const Integer q_ = Integer("0x3fffffffc0000000400000000000000000000000400000000000000000000000");
    static inline const Integer a_ = Integer("0xffffffff00000001000000000000000000000000fffffffffffffffffffffffc");
    static inline const Integer b_ = Integer("0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b");

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
    uint Size() { return this->size_; }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* EC_DATA_H_ */