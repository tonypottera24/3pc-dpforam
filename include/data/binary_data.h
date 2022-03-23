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
    std::vector<uchar> data_;

public:
    BinaryData();
    BinaryData(const BinaryData &other);
    BinaryData(const uint size);
    ~BinaryData();

    // inline std::vector<uchar> &GetData() { return this->data_; }
    // inline void SetData(std::vector<uchar> &data) { return this->data_; }

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
    void Load(std::vector<uchar> &data);
    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);
    uint Size() { return this->data_.size(); }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* BINARY_DATA_H_ */