#ifndef DATA_H_
#define DATA_H_

#include <inttypes.h>

#include "typedef.h"

class Data {
public:
    uint64_t size_;

public:
    Data();
    Data(const Data &other);
    Data(uchar *data, const uint size);
    Data(const uint size, const bool set_zero = false);
    ~Data();

    virtual Data &operator=(const Data &other) = 0;
    // Data &operator=(Data &&other) noexcept;
    virtual Data &operator+=(const Data &rhs) = 0;
    virtual Data &operator-=(const Data &rhs) = 0;
    // friend Data operator+(Data lhs, const Data &rhs);
    // friend Data operator-(Data lhs, const Data &rhs);
    virtual bool operator==(const Data &rhs) = 0;
    bool operator!=(const Data &rhs) { return !(*this == rhs); }

    virtual uint Size() = 0;
    virtual uchar *Dump() = 0;
    virtual void Load(uchar *data) = 0;
    virtual void Reset() = 0;
    // virtual void Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prg) = 0;
    // virtual void Random(CryptoPP::AutoSeededRandomPool *prg) = 0;
    virtual void Print(const char *title = "") = 0;
};

#endif /* DATA_H_ */