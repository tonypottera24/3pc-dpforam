#ifndef DATA_H_
#define DATA_H_

#include <inttypes.h>

class Data {
public:
    uint64_t size_;

public:
    Data Add(Data a, Data b);
    Data Minus(Data a, Data b);
};

#endif /* DATA_H_ */