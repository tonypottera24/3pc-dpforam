#include "data.h"

Data::Data(const Data &other) {
    // assert(this->data_type_ == NULL || this->data_type_ == other.data_type_);
    this->data_type_ = other.data_type_;
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ = BinaryData(other.binary_data_);
            break;
        case DataType::ZP:
            this->zp_data_ = ZpData(other.zp_data_);
            break;
    }
}

Data::Data(const DataType data_type) {
    this->data_type_ = data_type;
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ = BinaryData();
            break;
        case DataType::ZP:
            this->zp_data_ = ZpData();
            break;
    }
}

Data::Data(const DataType data_type, uchar *data, const uint size) {
    this->data_type_ = data_type;
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ = BinaryData(data, size);
            break;
        case DataType::ZP:
            this->zp_data_ = ZpData(data, size);
            break;
    }
}

Data::Data(const DataType data_type, const uint size, const bool set_zero) {
    this->data_type_ = data_type;
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ = BinaryData(size, set_zero);
            break;
        case DataType::ZP:
            this->zp_data_ = ZpData(size, set_zero);
            break;
    }
}

Data::~Data() {
}

Data &Data::operator=(const Data &other) {
    // copy operation
    // assert(this->data_type_ == other.data_type_);
    this->data_type_ = other.data_type_;
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ = other.binary_data_;
            break;
        case DataType::ZP:
            this->zp_data_ = other.zp_data_;
            break;
    }
    return *this;
}

Data &Data::operator+=(const Data &rhs) {
    assert(this->data_type_ == rhs.data_type_);
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ += rhs.binary_data_;
            break;
        case DataType::ZP:
            this->zp_data_ += rhs.zp_data_;
            break;
    }
    return *this;
}

Data &Data::operator-=(const Data &rhs) {
    assert(this->data_type_ == rhs.data_type_);
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_ -= rhs.binary_data_;
            break;
        case DataType::ZP:
            this->zp_data_ -= rhs.zp_data_;
            break;
    }
    return *this;
}

bool Data::operator==(const Data &rhs) {
    assert(this->data_type_ == rhs.data_type_);
    switch (this->data_type_) {
        case DataType::BINARY:
            return this->binary_data_ == rhs.binary_data_;
            break;
        case DataType::ZP:
            return this->zp_data_ == rhs.zp_data_;
            break;
    }
}

uint Data::Size() {
    switch (this->data_type_) {
        case DataType::BINARY:
            return this->binary_data_.Size();
            break;
        case DataType::ZP:
            return this->zp_data_.Size();
            break;
    }
}

bool Data::IsSymmetric() {
    switch (this->data_type_) {
        case DataType::BINARY:
            return this->binary_data_.IsSymmetric();
            break;
        case DataType::ZP:
            return this->zp_data_.IsSymmetric();
            break;
    }
}

uchar *Data::Dump() {
    switch (this->data_type_) {
        case DataType::BINARY:
            return this->binary_data_.Dump();
            break;
        case DataType::ZP:
            return this->zp_data_.Dump();
            break;
    }
}

void Data::Load(uchar *data) {
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_.Load(data);
            break;
        case DataType::ZP:
            this->zp_data_.Load(data);
            break;
    }
}

void Data::Reset() {
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_.Reset();
            break;
        case DataType::ZP:
            this->zp_data_.Reset();
            break;
    }
}

void Data::Random(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prg) {
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_.Random(prg);
            break;
        case DataType::ZP:
            this->zp_data_.Random(prg);
            break;
    }
}
void Data::Random(CryptoPP::AutoSeededRandomPool *prg) {
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_.Random(prg);
            break;
        case DataType::ZP:
            this->zp_data_.Random(prg);
            break;
    }
}

void Data::Print(const char *title) {
    switch (this->data_type_) {
        case DataType::BINARY:
            this->binary_data_.Print(title);
            break;
        case DataType::ZP:
            this->zp_data_.Print(title);
            break;
    }
}