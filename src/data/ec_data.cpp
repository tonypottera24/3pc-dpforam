#include "ec_data.h"

ECData::ECData() {
    this->data_ = this->group_.GetCurve().Identity();
}

ECData::ECData(uchar *data, const uint size) {
    this->group_.GetCurve().DecodePoint(this->data_, data, size);
}

ECData::ECData(const uint size, const bool set_zero) {
    if (set_zero) {
        this->Reset();
    }
}

ECData::ECData(const ECData &other) {
    this->data_ = other.data_;
}

ECData::~ECData() {
    // this->data_.~ECPPoint();
}

ECData &ECData::operator=(const ECData &other) {
    // copy operation
    if (this == &other) return *this;
    this->data_ = other.data_;
    return *this;
}

ECData ECData::operator-() {
    this->data_ = this->group_.GetCurve().Inverse(this->data_);
    return *this;
}

ECData &ECData::operator+=(const ECData &rhs) {
    this->data_ = this->group_.GetCurve().Add(this->data_, rhs.data_);
    return *this;
}

ECData &ECData::operator-=(const ECData &rhs) {
    this->data_ = this->group_.GetCurve().Subtract(this->data_, rhs.data_);
    return *this;
}

bool ECData::operator==(const ECData &rhs) {
    return this->data_ == rhs.data_;
}

uchar *ECData::Dump() {
    uchar *data_bytes = new uchar[this->Size()];
    this->group_.GetCurve().EncodePoint(data_bytes, this->data_, this->compressed_);
    return data_bytes;
}

void ECData::Load(uchar *data, uint size) {
    this->group_.GetCurve().DecodePoint(this->data_, data, size);
}

void ECData::Reset() {
    this->data_ = this->group_.GetCurve().Identity();
}

void ECData::Random(uint size) {
    AutoSeededRandomPool prng;
    Integer x(prng, Integer::One(), this->group_.GetMaxExponent());
    this->data_ = this->group_.ExponentiateBase(x);
    // this->data_ = this->group_.GetCurve().Identity();
}

void ECData::Random(CTR_Mode<AES>::Encryption &prg, uint size) {
    Integer x(prg, Integer::One(), this->group_.GetMaxExponent());
    this->data_ = this->group_.ExponentiateBase(x);
    // this->data_ = this->group_.GetCurve().Identity();
}

void ECData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%llu\n", this->data_);
}