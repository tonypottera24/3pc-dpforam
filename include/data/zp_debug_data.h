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
    ZpDebugData() {
    }

    ZpDebugData(const uint size) {
    }

    ZpDebugData(const ZpDebugData &other) {
        this->data_ = other.data_ % this->p_;
    }

    ~ZpDebugData() {
    }

    ZpDebugData inline &operator=(const ZpDebugData &other) {
        // copy operation
        if (this == &other) return *this;
        this->data_ = other.data_;
        return *this;
    }

    ZpDebugData inline operator-() {
        this->data_ = (this->p_ - this->data_) % this->p_;
        return *this;
    }

    ZpDebugData inline &operator+=(const ZpDebugData &rhs) {
        this->data_ = (this->data_ + rhs.data_) % this->p_;
        return *this;
    }

    ZpDebugData inline &operator-=(const ZpDebugData &rhs) {
        if (this->data_ > rhs.data_) {
            this->data_ -= rhs.data_;
        } else {
            this->data_ = (this->data_ + (this->p_ - rhs.data_)) % this->p_;
        }
        return *this;
    }

    bool inline operator==(const ZpDebugData &rhs) {
        return this->data_ == rhs.data_;
    }

    ZpDebugData inline operator+(const ZpDebugData &rhs) const {
        ZpDebugData result;
        result.data_ = (this->data_ + rhs.data_) % this->p_;
        return result;
    }

    ZpDebugData inline operator-(const ZpDebugData &rhs) const {
        ZpDebugData result;
        if (this->data_ > rhs.data_) {
            result.data_ = this->data_ - rhs.data_;
        } else {
            result.data_ = (this->data_ + (this->p_ - rhs.data_)) % this->p_;
        }
        return result;
    }

    // friend ZpDebugData operator+(ZpDebugData lhs, const ZpDebugData &rhs);
    // friend ZpDebugData operator-(ZpDebugData lhs, const ZpDebugData &rhs);

    std::vector<uchar> inline Dump() {
        std::vector<uchar> data(sizeof(uint));
        memcpy(data.data(), (uchar *)&(this->data_), sizeof(uint));
        return data;
    }

    void inline Load(std::vector<uchar> &data) {
        memcpy((uchar *)&(this->data_), data.data(), data.size());
    }

    void inline Reset() {
        this->data_ = 0;
    }

    void inline Resize(const uint size) {
    }

    void inline Random(PRG *prg = NULL) {
        if (prg == NULL) prg = this->prg_;
        this->data_ = rand_uint(prg) % this->p_;
        // fprintf(stderr, "rand %u\n", this->data_);
    }

    uint inline Size() { return sizeof(uint); }
    static inline bool IsSymmetric() { return is_symmetric_; }
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