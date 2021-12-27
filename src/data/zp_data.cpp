#include "zp_data.h"

ZpData::ZpData() {
    this->data_ = 0;
}

ZpData::ZpData(uchar *data, const uint size) {
    memcpy((uchar *)&(this->data_), data, this->size_);
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

// ZpData &ZpData::operator=(ZpData &&other) noexcept {
//     // move operation
//     if (this == &other) {
//         return *this;
//     }

//     delete[] this->data_;
//     this->data_ = std::exchange(other.data_, nullptr);
//     this->size_ = std::exchange(other.size_, 0);
//     return *this;
// }

ZpData ZpData::operator-() {
    ZpData *z = new ZpData(this->size_);
    z->data_ = (this->p_ - this->data_) % this->p_;
    return *z;
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
    uchar *data_bytes = new uchar[this->size_];
    memcpy(data_bytes, (uchar *)&(this->data_), this->size_);
    return data_bytes;
}

void ZpData::Load(uchar *data, uint size) {
    memcpy((uchar *)&(this->data_), data, size);
    this->data_ %= this->p_;
}

// void ZpData::ConvertFromBytes(uchar *data, uint size) {
//     // memcpy((uchar *)&(this->data_), data, this->size_);
//     CryptoPP::AutoSeededRandomPool rnd;
//     if (this->q_) {
//         this->data_ %= this->p_;
//     } else {
//         rnd.GenerateBlock((uchar *)&(this->data_), this->size_);
//     }
// }

void ZpData::Reset() {
    this->data_ = 0;
}

void ZpData::Random(uint size) {
    CryptoPP::AutoSeededRandomPool rnd;
    rnd.GenerateBlock((uchar *)&(this->data_), this->size_);
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