#ifndef DATA_H_
#define DATA_H_

#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <inttypes.h>

#include "binary_data.h"
#include "typedef.h"
#include "zp_data.h"

enum class DataType {
    BINARY,
    ZP,
};

class Data {
protected:
    BinaryData binary_data_;
    ZpData zp_data_;

public:
    DataType data_type_;

public:
    Data() {}
    Data(const Data &other);
    Data(const DataType data_type);
    Data(const DataType data_type, uchar *data, const uint size);
    Data(const DataType data_type, const uint size, const bool set_zero = false);
    ~Data();

    Data &operator=(const Data &other);
    // Data &operator=(Data &&other) noexcept;
    Data &operator+=(const Data &rhs);
    Data &operator-=(const Data &rhs);
    friend Data operator+(Data lhs, const Data &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend Data operator-(Data lhs, const Data &rhs) {
        lhs -= rhs;
        return lhs;
    }
    bool operator==(const Data &rhs);
    bool operator!=(const Data &rhs) { return !(*this == rhs); }

    uint Size();
    bool IsSymmetric();
    uchar *Dump();
    void Load(uchar *data);
    void Reset();
    void Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prg);
    void Random(CryptoPP::AutoSeededRandomPool *prg);
    void Print(const char *title = "");
};

#endif /* DATA_H_ */