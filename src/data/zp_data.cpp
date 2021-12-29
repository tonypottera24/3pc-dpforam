#include "zp_data.h"

ZpData::ZpData() {
    this->data_ = 0;
}

ZpData::ZpData(uchar *data, const uint size) {
    memcpy((uchar *)&(this->data_), data, this->Size());
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
    this->data_ = 0;
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

uchar *ZpData::Dump() {
    uchar *data_bytes = new uchar[this->Size()];
    memcpy(data_bytes, (uchar *)&(this->data_), this->Size());
    return data_bytes;
}

void ZpData::Load(uchar *data, uint size) {
    memcpy((uchar *)&(this->data_), data, size);
    this->data_ %= this->p_;
}

void ZpData::Reset() {
    this->data_ = 0;
}

void ZpData::Random(uint size) {
    CryptoPP::AutoSeededRandomPool rnd;
    rnd.GenerateBlock((uchar *)&(this->data_), this->Size());
    this->data_ %= this->p_;
}

void ZpData::Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &prg, uint size) {
    prg.GenerateBlock((uchar *)&(this->data_), size);
    this->data_ %= this->p_;
}

void ZpData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%llu\n", this->data_);
}