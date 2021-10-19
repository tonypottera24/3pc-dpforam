#ifndef DATA_H_
#define DATA_H_

#include <inttypes.h>

class Data {
public:
    uint64_t size_;

public:
    virtual Data &operator=(const Data &other) = 0;
    virtual Data &operator+=(const Data &rhs) = 0;
    virtual Data &operator-=(const Data &rhs) = 0;
    // friend Data operator+(Data lhs, const Data &rhs);
    // friend Data operator-(Data lhs, const Data &rhs);
};

#endif /* DATA_H_ */