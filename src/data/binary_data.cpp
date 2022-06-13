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
    neg_bytes(this->data_.data(), this->data_.data(), this->data_.size());
    return *this;
}

BinaryData &BinaryData::operator+=(const BinaryData &rhs) {
    // assert(this->data_.size() == rhs.data_.size());
    xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
    return *this;
}

BinaryData &BinaryData::operator-=(const BinaryData &rhs) {
    // assert(this->data_.size() == rhs.data_.size());
    xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
    return *this;
}

bool BinaryData::operator==(const BinaryData &rhs) {
    return this->data_.size() == rhs.data_.size() &&
           memcmp(this->data_.data(), rhs.data_.data(), this->data_.size()) == 0;
}

void BinaryData::DumpBuffer(uchar *buffer) {
    memcpy(buffer, this->data_.data(), this->data_.size());
}

std::vector<uchar> BinaryData::DumpVector() {
    return this->data_;
}

void BinaryData::LoadBuffer(uchar *buffer) {
    memcpy(this->data_.data(), buffer, this->data_.size());
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