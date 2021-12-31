#include "binary_data.h"

BinaryData::BinaryData() {
}

BinaryData::BinaryData(uchar *data, const uint size) {
    if (size == 0) return;
    this->Resize(size);
    memcpy(this->data_, data, this->size_);
}

BinaryData::BinaryData(const uint size, const bool set_zero) {
    if (size == 0) return;
    this->Resize(size);
    if (set_zero) {
        this->Reset();
    }
}

BinaryData::BinaryData(const BinaryData &other) {
    if (other.size_ == 0) return;
    this->Resize(other.size_);
    memcpy(this->data_, other.data_, other.size_);
}

BinaryData::~BinaryData() {
    if (this->data_ == NULL) return;
    delete[] this->data_;
}

BinaryData &BinaryData::operator=(const BinaryData &other) {
    // copy operation
    if (this == &other) return *this;
    this->Resize(other.size_);
    if (other.size_ > 0) {
        memcpy(this->data_, other.data_, other.size_);
    }
    return *this;
}

// BinaryData &BinaryData::operator=(BinaryData &&other) noexcept {
//     // move operation
//     if (this == &other) {
//         return *this;
//     }

//     delete[] this->data_;
//     this->data_ = std::exchange(other.data_, nullptr);
//     this->size_ = std::exchange(other.size_, 0);
//     return *this;
// }

BinaryData BinaryData::operator-() {
    for (uint i = 0; i < this->size_; i++) {
        this->data_[i] = ~this->data_[i];
    }
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
    return this->size_ == rhs.size_ && memcmp(this->data_, rhs.data_, size_) == 0;
}

uchar *BinaryData::Dump() {
    uchar *data_bytes = new uchar[this->size_];
    memcpy(data_bytes, this->data_, this->size_);
    return data_bytes;
}

void BinaryData::Load(uchar *data, uint size) {
    this->Resize(size);
    if (size > 0) {
        memcpy(this->data_, data, size);
    }
}

void BinaryData::Reset() {
    if (this->data_ != NULL) {
        memset(this->data_, 0, this->size_);
    }
}

void BinaryData::Random(uint size) {
    CryptoPP::AutoSeededRandomPool prg;
    this->Random(prg, size);
}

void BinaryData::Random(CryptoPP::RandomNumberGenerator &prg, uint size) {
    this->Resize(size);
    if (size > 0) {
        prg.GenerateBlock(this->data_, size);
    }
}

void BinaryData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("0x");
    for (uint i = 0; i < this->size_; i++) {
        debug_print("%02X", this->data_[i]);
    }
    debug_print("\n");
}

void BinaryData::Resize(uint size) {
    if (this->size_ == size) return;
    if (this->data_ != NULL) {
        delete[] this->data_;
        this->data_ = NULL;
    }
    if (size > 0) this->data_ = new uchar[size];
    this->size_ = size;
}