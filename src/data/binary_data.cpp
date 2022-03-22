#include "binary_data.h"

BinaryData::BinaryData() {
}

BinaryData::BinaryData(const uint size) {
    this->Resize(size);
    this->Reset();
}

BinaryData::BinaryData(const BinaryData &other) {
    this->data_ = other.data_;
}

BinaryData::~BinaryData() {
    // this->data_.clear();
}

BinaryData &BinaryData::operator=(const BinaryData &other) {
    // copy operation
    if (this == &other) return *this;
    this->data_ = other.data_;
    return *this;
}

BinaryData BinaryData::operator-() {
    for (uint i = 0; i < this->data_.size(); i++) {
        this->data_[i] = ~this->data_[i];
    }
    return *this;
}

void BinaryData::Add(const BinaryData &a, const BinaryData &b, BinaryData &r) {
    assert(a.data_.size() == b.data_.size() && a.data_.size() == r.data_.size());
    xor_bytes(a.data_.data(), b.data_.data(), r.data_.data(), a.data_.size());
}

void BinaryData::Minus(const BinaryData &a, const BinaryData &b, BinaryData &r) {
    assert(a.data_.size() == b.data_.size() && a.data_.size() == r.data_.size());
    xor_bytes(a.data_.data(), b.data_.data(), r.data_.data(), a.data_.size());
}

bool BinaryData::operator==(const BinaryData &rhs) {
    return this->data_.size() == rhs.data_.size() &&
           memcmp(this->data_.data(), rhs.data_.data(), this->data_.size()) == 0;
}

void BinaryData::Dump(std::vector<uchar> &data) {
    data = this->data_;
}

void BinaryData::Load(std::vector<uchar> &data) {
    this->data_ = data;
}

void BinaryData::Reset() {
    if (this->data_.size() > 0) {
        memset(this->data_.data(), 0, this->data_.size());
    }
}

void BinaryData::Resize(const uint size) {
    this->data_.resize(size);
}

void BinaryData::Random(PRG *prg) {
    if (prg == NULL) prg = this->prg_;
    if (this->data_.size() > 0) {
        prg->RandBytes(this->data_.data(), this->data_.size());
    }
}

void BinaryData::Print(const char *title) {
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