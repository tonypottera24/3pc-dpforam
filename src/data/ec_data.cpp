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
    AutoSeededRandomPool prg;
    this->Random(prg, size);
}

void ECData::Random(RandomNumberGenerator &prg, uint size) {
    Integer x(prg, Integer::One(), this->p_ - Integer::One());
    bool v = x.GetBit(0);
    while (true) {
        Integer r = ((a_exp_b_mod_c(x, Integer("3"), this->p_) + (this->a_ * x) % this->p_) % this->p_ + this->b_) % this->p_;
        Integer y = a_exp_b_mod_c(r, this->q_, this->p_);
        if (a_exp_b_mod_c(y, Integer::Two(), this->p_) == r) {
            if (v) {
                this->data_ = ECP::Point(x, y);
            } else {
                this->data_ = ECP::Point(x, (this->p_ - y) % this->p_);
            }
            break;
        }
        x += 1;
    }
    // Integer x(prg, Integer::One(), this->group_.GetMaxExponent());
    // this->data_ = this->group_.ExponentiateBase(x);
    // this->data_ = this->group_.GetCurve().Identity();
}

void ECData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%llu\n", this->data_);
}