#include "binary_data.h"

BinaryData::BinaryData(uchar* data, uint64_t size) {
    this->data_ = data;
    this->size_ = size;
}

BinaryData::~BinaryData() {
    delete[] this->data_;
}

BinaryData BinaryData::Add(BinaryData a, BinaryData b) {
    assert(a.size_ == b.size_);
    uchar* out = new uchar[a.size_];
    xor_bytes(a.data_, b.data_, a.size_, out);
    return BinaryData(out, a.size_);
}

BinaryData BinaryData::Minus(BinaryData a, BinaryData b) {
    return this->Add(a, b);
}