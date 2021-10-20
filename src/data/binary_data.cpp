#include "binary_data.h"

BinaryData::BinaryData() {
    this->data_ = NULL;
    this->size_ = 0;
}

BinaryData::BinaryData(uchar *data, const uint size) {
    this->data_ = new uchar[size];
    this->size_ = size;
    memcpy(this->data_, data, this->size_);
}

BinaryData::BinaryData(const uint size, const bool set_zero) {
    this->data_ = new uchar[size];
    this->size_ = size;
    if (set_zero) {
        this->Reset();
    }
}

BinaryData::BinaryData(const BinaryData &other) {
    // NOTE test this part
    // this->data_ = other.data_;
    // this->size_ = other.size_;
    this->data_ = new uchar[other.size_];
    memcpy(this->data_, other.data_, other.size_);
    this->size_ = other.size_;
}

BinaryData::~BinaryData() {
    delete[] this->data_;
}

BinaryData &BinaryData::operator=(const BinaryData &other) {
    // copy operation
    if (this == &other) {
        return *this;
    }

    this->data_ = new uchar[other.size_];
    memcpy(this->data_, other.data_, other.size_);
    this->size_ = other.size_;

    return *this;
}

BinaryData &BinaryData::operator=(BinaryData &&other) noexcept {
    // move operation
    if (this == &other) {
        return *this;
    }

    delete[] this->data_;
    this->data_ = std::exchange(other.data_, nullptr);
    this->size_ = std::exchange(other.size_, 0);
    return *this;
}

BinaryData &BinaryData::operator+=(const BinaryData &rhs) {
    assert(this->size_ == rhs.size_);
    xor_bytes(this->data_, rhs.data_, rhs.size_, this->data_);
    return *this;
}

BinaryData &BinaryData::operator-=(const BinaryData &rhs) {
    assert(this->size_ == rhs.size_);
    xor_bytes(this->data_, rhs.data_, rhs.size_, this->data_);
    return *this;
}

bool BinaryData::operator==(const BinaryData &rhs) {
    return this->size_ == rhs.size_ && (memcmp(this->data_, rhs.data_, size_) == 0);
}

uint BinaryData::Size() {
    return this->size_;
}

uchar *BinaryData::Dump() {
    return this->data_;
}

void BinaryData::Load(uchar *data) {
    memcpy(this->data_, data, this->size_);
}

void BinaryData::Reset() {
    memset(this->data_, 0, this->size_);
}

void BinaryData::Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &prg) {
    prg.GenerateBlock(this->data_, this->size_);
}
void BinaryData::Random(CryptoPP::AutoSeededRandomPool &prg) {
    prg.GenerateBlock(this->data_, this->size_);
}

void BinaryData::Print(const char *title) {
    if (strlen(title) > 0) {
        fprintf(stderr, "%s ", title);
    }
    fprintf(stderr, "0x");
    for (uint i = 0; i < this->size_; i++) {
        fprintf(stderr, "%02X", this->data_[i]);
    }
    fprintf(stderr, "\n");
}