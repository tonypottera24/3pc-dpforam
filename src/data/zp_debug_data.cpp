#include "zp_debug_data.h"

ZpDebugData::ZpDebugData() {}
ZpDebugData::ZpDebugData(const uint size) {}

ZpDebugData::ZpDebugData(const ZpDebugData &other) {
    this->data_ = other.data_ % this->p_;
}

ZpDebugData::~ZpDebugData() {}

ZpDebugData &ZpDebugData::operator=(const ZpDebugData &other) {
    // copy operation
    if (this == &other) return *this;
    this->data_ = other.data_;
    return *this;
}

ZpDebugData ZpDebugData::operator-() {
    this->data_ = (this->p_ - this->data_) % this->p_;
    return *this;
}

ZpDebugData &ZpDebugData::operator+=(const ZpDebugData &rhs) {
    this->data_ = (this->data_ + rhs.data_) % this->p_;
    return *this;
}

ZpDebugData &ZpDebugData::operator-=(const ZpDebugData &rhs) {
    if (this->data_ > rhs.data_) {
        this->data_ -= rhs.data_;
    } else {
        this->data_ = (this->data_ + (this->p_ - rhs.data_)) % this->p_;
    }
    return *this;
}

bool ZpDebugData::operator==(const ZpDebugData &rhs) {
    return this->data_ == rhs.data_;
}

void ZpDebugData::DumpBuffer(uchar *buffer) {
    memcpy(buffer, &(this->data_), sizeof(uint));
}

std::vector<uchar> ZpDebugData::DumpVector() {
    std::vector<uchar> data(sizeof(uint));
    DumpBuffer(data.data());
    return data;
}

void ZpDebugData::LoadBuffer(uchar *buffer) {
    memcpy(&(this->data_), buffer, sizeof(uint));
}

void ZpDebugData::Reset() {
    this->data_ = 0;
}

void ZpDebugData::Resize(const uint size) {
}

void ZpDebugData::Random(PRG *prg) {
    if (prg == NULL) prg = this->prg_;
    this->data_ = rand_uint(prg) % this->p_;
}

void ZpDebugData::Print(const char *title) {
#ifdef DEBUG
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%u\n", this->data_);
#endif
}