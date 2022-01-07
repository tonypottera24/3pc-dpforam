#include "debug_zp_data.h"

DebugZpData::DebugZpData() {
    this->data_ = 0;
}

DebugZpData::DebugZpData(uchar *data, const uint size) {
    memcpy((uchar *)&(this->data_), data, this->Size());
}

DebugZpData::DebugZpData(const uint size, const bool set_zero) {
    if (set_zero) {
        this->Reset();
    }
}

DebugZpData::DebugZpData(const DebugZpData &other) {
    this->data_ = other.data_ % this->p_;
}

DebugZpData::~DebugZpData() {
    this->data_ = 0;
}

DebugZpData &DebugZpData::operator=(const DebugZpData &other) {
    // copy operation
    if (this == &other) return *this;
    this->data_ = other.data_;
    return *this;
}

DebugZpData DebugZpData::operator-() {
    this->data_ = (this->p_ - this->data_) % this->p_;
    return *this;
}

DebugZpData &DebugZpData::operator+=(const DebugZpData &rhs) {
    this->data_ = (this->data_ + rhs.data_) % this->p_;
    return *this;
}

DebugZpData &DebugZpData::operator-=(const DebugZpData &rhs) {
    this->data_ = ((this->data_ + this->p_) - rhs.data_) % this->p_;
    return *this;
}

bool DebugZpData::operator==(const DebugZpData &rhs) {
    return (this->data_ % this->p_) == (rhs.data_ % this->p_);
}

uchar *DebugZpData::Dump() {
    uchar *data_bytes = new uchar[this->Size()];
    memcpy(data_bytes, (uchar *)&(this->data_), this->Size());
    return data_bytes;
}

void DebugZpData::Load(uchar *data, uint size) {
    memcpy((uchar *)&(this->data_), data, size);
    this->data_ %= this->p_;
}

void DebugZpData::Reset() {
    this->data_ = 0;
}

void DebugZpData::Random(uint size) {
    CryptoPP::AutoSeededRandomPool prg;
    this->Random(prg, size);
}

void DebugZpData::Random(CryptoPP::RandomNumberGenerator &prg, uint size) {
    prg.GenerateBlock((uchar *)&(this->data_), this->Size());
    this->data_ %= this->p_;
}

void DebugZpData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%llu\n", this->data_);
}