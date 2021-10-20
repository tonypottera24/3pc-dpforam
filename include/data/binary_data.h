#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include "typedef.h"
#include "util.h"

class BinaryData {
private:
    uint size_;
    uchar *data_;

public:
    BinaryData();
    BinaryData(const BinaryData &other);
    BinaryData(uchar *data, const uint size);
    BinaryData(const uint size, const bool set_zero = false);
    ~BinaryData();

    BinaryData &operator=(const BinaryData &other);
    // BinaryData &operator=(BinaryData &&other) noexcept;
    BinaryData &operator+=(const BinaryData &rhs);
    BinaryData &operator-=(const BinaryData &rhs);
    bool operator==(const BinaryData &rhs);

    uchar *Dump();
    void Load(uchar *data);
    void Reset();
    void Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prg);
    void Random(CryptoPP::AutoSeededRandomPool *prg);
    uint Size() { return this->size_; }
    void Print(const char *title = "");
};

#endif /* BINARY_DATA_H_ */