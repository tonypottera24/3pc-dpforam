#ifndef BINARY_DATA_H_
#define BINARY_DATA_H_

#include <inttypes.h>

#include "prg.h"
#include "typedef.h"
#include "util.h"

class BinaryData {
private:
    static const bool is_symmetric_ = true;
    static inline PRG *prg_ = new PRG();
    std::vector<uchar> data_;

public:
    BinaryData() {
    }

    BinaryData(const uint size) {
        this->Resize(size);
        this->Reset();
    }

    BinaryData(const BinaryData &other) {
        this->data_ = other.data_;
    }

    ~BinaryData() {
        // this->data_.clear();
    }

    BinaryData inline &operator=(const BinaryData &other) {
        // copy operation
        if (this == &other) return *this;
        this->data_ = other.data_;
        return *this;
    }

    BinaryData inline operator-() {
        neg_bytes(this->data_.data(), this->data_.data(), this->data_.size());
        return *this;
    }

    BinaryData inline &operator+=(const BinaryData &rhs) {
        // assert(this->data_.size() == rhs.data_.size());
        xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
        return *this;
    }

    BinaryData inline &operator-=(const BinaryData &rhs) {
        // assert(this->data_.size() == rhs.data_.size());
        xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
        return *this;
    }

    bool inline operator==(const BinaryData &rhs) {
        return this->data_.size() == rhs.data_.size() &&
               memcmp(this->data_.data(), rhs.data_.data(), this->data_.size()) == 0;
    }

    friend inline BinaryData operator+(BinaryData lhs, const BinaryData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend inline BinaryData operator-(BinaryData lhs, const BinaryData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void inline DumpBuffer(uchar *buffer) {
        memcpy(buffer, this->data_.data(), this->data_.size());
    }

    std::vector<uchar> inline DumpVector() {
        return this->data_;
    }

    void inline LoadBuffer(uchar *buffer) {
        memcpy(this->data_.data(), buffer, this->data_.size());
    }

    void inline LoadVector(std::vector<uchar> &data) {
        this->data_ = data;
    }

    void inline Reset() {
        if (this->data_.size() > 0) {
            memset(this->data_.data(), 0, this->data_.size());
        }
    }

    void inline Resize(const uint size) {
        this->data_.resize(size);
    }

    void inline Random(PRG *prg = NULL) {
        if (prg == NULL) prg = this->prg_;
        if (this->data_.size() > 0) {
            prg->RandBytes(this->data_.data(), this->data_.size());
        }
    }

    uint inline Size() { return this->data_.size(); }
    static bool inline IsSymmetric() { return is_symmetric_; }

    void inline Print(const char *title = "") {
#ifdef DEBUG
        if (strlen(title) > 0) {
            debug_print("%s ", title);
        }
        debug_print("0x");
        for (uint i = 0; i < this->data_.size(); i++) {
            debug_print("%02X", this->data_[i]);
        }
        debug_print("\n");
#endif
    }
};

#endif /* BINARY_DATA_H_ */