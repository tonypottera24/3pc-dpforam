#ifndef ZP_DEBUG_DATA_H_
#define ZP_DEBUG_DATA_H_

#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpDebugData {
private:
    uint data_ = 0;
    // const static uint p_ = 4294967291;
    const static uint p_ = 2147483647;
    static const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ZpDebugData() {}
    ZpDebugData(const uint size) {}

    ZpDebugData(const ZpDebugData &other) {
        this->data_ = other.data_ % this->p_;
    }

    ~ZpDebugData() {}

    ZpDebugData &operator=(const ZpDebugData &other) {
        // copy operation
        if (this == &other) return *this;
        this->data_ = other.data_;
        return *this;
    }

    ZpDebugData operator-() {
        this->data_ = (this->p_ - this->data_) % this->p_;
        return *this;
    }

    ZpDebugData &operator+=(const ZpDebugData &rhs) {
        this->data_ = (this->data_ + rhs.data_) % this->p_;
        return *this;
    }

    ZpDebugData &operator-=(const ZpDebugData &rhs) {
        if (this->data_ > rhs.data_) {
            this->data_ -= rhs.data_;
        } else {
            this->data_ = (this->data_ + (this->p_ - rhs.data_)) % this->p_;
        }
        return *this;
    }

    bool operator==(const ZpDebugData &rhs) {
        return this->data_ == rhs.data_;
    }

    friend ZpDebugData operator+(ZpDebugData lhs, const ZpDebugData &rhs) {
        lhs += rhs;
        return lhs;
    }

    friend ZpDebugData operator-(ZpDebugData lhs, const ZpDebugData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void DumpBuffer(uchar *buffer) {
        memcpy(buffer, &(this->data_), sizeof(uint));
    }

    std::vector<uchar> DumpVector() {
        std::vector<uchar> data(sizeof(uint));
        DumpBuffer(data.data());
        return data;
    }

    void LoadBuffer(uchar *buffer) {
        memcpy(&(this->data_), buffer, sizeof(uint));
    }

    void LoadVector(std::vector<uchar> &data) {
        LoadBuffer(data.data());
    }

    void Reset() {
        this->data_ = 0;
    }

    void Resize(const uint size) {
    }

    void Random(PRG *prg = NULL) {
        if (prg == NULL) prg = this->prg_;
        this->data_ = rand_uint(prg) % this->p_;
        // fprintf(stderr, "rand %u\n", this->data_);
    }

    uint Size() { return sizeof(uint); }

    static bool IsSymmetric() { return is_symmetric_; }

    void Print(const char *title = "") {
#ifdef DEBUG
        if (strlen(title) > 0) {
            debug_print("%s ", title);
        }
        debug_print("%u\n", this->data_);
#endif
    }
};

#endif /* ZP_DEBUG_DATA_H_ */