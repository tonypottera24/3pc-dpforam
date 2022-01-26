#include "zp_data.h"

ZpData::ZpData() {
    this->data_ = Integer::Zero();
}

ZpData::ZpData(uchar *data, const uint size) {
    this->data_.Decode(data, size);
}

ZpData::ZpData(const uint size, const bool set_zero) {
    if (set_zero) {
        this->Reset();
    }
}

ZpData::ZpData(const ZpData &other) {
    this->data_ = other.data_ % this->p_;
}

ZpData::~ZpData() {
}

ZpData &ZpData::operator=(const ZpData &other) {
    // copy operation
    if (this == &other) return *this;
    this->data_ = other.data_;
    return *this;
}

ZpData ZpData::operator-() {
    this->data_ = (this->p_ - this->data_) % this->p_;
    return *this;
}

ZpData &ZpData::operator+=(const ZpData &rhs) {
    this->data_ = (this->data_ + rhs.data_) % this->p_;
    return *this;
}

ZpData &ZpData::operator-=(const ZpData &rhs) {
    this->data_ = ((this->data_ + this->p_) - rhs.data_) % this->p_;
    return *this;
}

bool ZpData::operator==(const ZpData &rhs) {
    return (this->data_ % this->p_) == (rhs.data_ % this->p_);
}

void ZpData::Dump(uchar *data) {
    this->data_.Encode(data, this->Size());
}

void ZpData::Load(uchar *data, uint size) {
    this->data_.Decode(data, size);
}

void ZpData::Reset() {
    this->data_ = Integer::Zero();
}

void ZpData::Random(uint size) {
    AutoSeededRandomPool prg;
    this->Random(prg, size);
}

void ZpData::Random(RandomNumberGenerator &prg, uint size) {
    Integer x(prg, Integer::Zero(), this->p_ - Integer::One());
    this->data_ = x;
}

void ZpData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    // debug_print("%llu\n", this->data_);
}