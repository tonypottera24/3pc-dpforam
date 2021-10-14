#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include "data.h"
#include "typedef.h"
#include "util.h"

class BinaryData : public Data {
private:
    uchar* data_;

public:
    BinaryData(uchar* data, uint64_t size);
    ~BinaryData();
    BinaryData Add(BinaryData a, BinaryData b);
    BinaryData Minus(BinaryData a, BinaryData b);
};

#endif /* BINARY_DATA_H_ */