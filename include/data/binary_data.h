#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class BinaryData {
private:
    uint size_ = 0;
    uchar *data_ = NULL;
    const bool is_symmetric_ = true;
    void Resize(uint size);

public:
    BinaryData();
    BinaryData(const BinaryData &other);
    BinaryData(uchar *data, const uint size);
    BinaryData(const uint size, const bool set_zero = false);
    ~BinaryData();

    BinaryData &operator=(const BinaryData &other);
    // BinaryData &operator=(BinaryData &&other) noexcept;
    BinaryData operator-();
    BinaryData &operator+=(const BinaryData &rhs);
    BinaryData &operator-=(const BinaryData &rhs);
    bool operator==(const BinaryData &rhs);
    friend BinaryData operator+(BinaryData lhs, const BinaryData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend BinaryData operator-(BinaryData lhs, const BinaryData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void Dump(uchar *data);
    void Load(uchar *data, uint size);
    void Reset();
    void Random(uint size);
    void Random(CryptoPP::RandomNumberGenerator &prg, uint size);
    uint Size() { return this->size_; }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* BINARY_DATA_H_ */