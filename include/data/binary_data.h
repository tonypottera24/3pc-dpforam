#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include <inttypes.h>

#include "prg.h"
#include "typedef.h"
#include "util.h"

class BinaryData {
private:
    const bool is_symmetric_ = true;
    static inline PRG *prg_ = new PRG();

public:
    std::vector<uchar> data_;

    BinaryData();
    BinaryData(const BinaryData &other);
    BinaryData(const uint size);
    ~BinaryData();

    BinaryData &operator=(const BinaryData &other);
    // BinaryData &operator=(BinaryData &&other) noexcept;
    BinaryData operator-();
    bool operator==(const BinaryData &rhs);

    static void Add(const BinaryData &a, const BinaryData &b, BinaryData &r);
    static void Minus(const BinaryData &a, const BinaryData &b, BinaryData &r);

    void Dump(std::vector<uchar> &data);
    void Load(std::vector<uchar> &data);
    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);
    uint Size() { return this->data_.size(); }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* BINARY_DATA_H_ */