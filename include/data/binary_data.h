#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include <inttypes.h>

#include "benchmark/constant.h"
#include "prg.h"
#include "typedef.h"
#include "util.h"

class BinaryData {
private:
    static const bool is_symmetric_ = true;
    static inline PRG *prg_ = new PRG();
    std::vector<uchar> data_;

public:
    BinaryData();
    BinaryData(const uint size);
    BinaryData(const BinaryData &other);
    ~BinaryData();

    BinaryData &operator=(const BinaryData &other);

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

    void DumpBuffer(uchar *buffer);
    std::vector<uchar> DumpVector();
    void LoadBuffer(uchar *buffer);

    uint64_t hash(uint64_t digest_n, int b);

    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);

    uint Size() { return this->data_.size(); }
    static bool IsSymmetric() { return is_symmetric_; }

    void Print(const char *title = "");
};

#endif /* BINARY_DATA_H_ */