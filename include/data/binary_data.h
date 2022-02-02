#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include <inttypes.h>

#include "prg.h"
#include "typedef.h"
#include "util.h"

class BinaryData {
private:
    std::vector<uchar> data_;
    const bool is_symmetric_ = true;
    static inline PRG *prg_ = new PRG();

public:
    BinaryData();
    BinaryData(const BinaryData &other);
    BinaryData(const uint size);
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

    std::vector<uchar> Dump();
    void Load(std::vector<uchar> data);
    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);
    uint Size() { return this->data_.size(); }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* BINARY_DATA_H_ */