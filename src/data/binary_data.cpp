#include "binary_data.h"

BinaryData::BinaryData() {
}

BinaryData::BinaryData(const uint size) {
    this->Resize(size);
    this->Reset();
}

BinaryData::BinaryData(const BinaryData &other) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_COPY.Start();
#endif
    this->data_ = other.data_;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_COPY.End();
#endif
}

BinaryData::~BinaryData() {
    // this->data_.clear();
}

BinaryData &BinaryData::operator=(const BinaryData &other) {
    // copy operation
    if (this == &other) return *this;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_COPY.Start();
#endif
    this->data_ = other.data_;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_COPY.End();
#endif
    return *this;
}

BinaryData BinaryData::operator-() {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_ARITHMATIC.Start();
#endif
    neg_bytes(this->data_.data(), this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_ARITHMATIC.End();
#endif
    return *this;
}

BinaryData &BinaryData::operator+=(const BinaryData &rhs) {
    // assert(this->data_.size() == rhs.data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_ARITHMATIC.Start();
#endif
    xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_ARITHMATIC.End();
#endif
    return *this;
}

BinaryData &BinaryData::operator-=(const BinaryData &rhs) {
    // assert(this->data_.size() == rhs.data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_ARITHMATIC.Start();
#endif
    xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_ARITHMATIC.End();
#endif
    return *this;
}

bool BinaryData::operator==(const BinaryData &rhs) {
    return this->data_.size() == rhs.data_.size() &&
           memcmp(this->data_.data(), rhs.data_.data(), this->data_.size()) == 0;
}

void BinaryData::DumpBuffer(uchar *buffer) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_DUMP_LOAD.Start();
#endif
    memcpy(buffer, this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_DUMP_LOAD.End();
#endif
}

std::vector<uchar> BinaryData::DumpVector() {
    return this->data_;
}

void BinaryData::LoadBuffer(uchar *buffer) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_DUMP_LOAD.Start();
#endif
    memcpy(this->data_.data(), buffer, this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_DUMP_LOAD.End();
#endif
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
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_RANDOM.Start();
#endif
    if (prg == NULL) prg = this->prg_;
    if (this->data_.size() > 0) {
        prg->RandBytes(this->data_.data(), this->data_.size());
    }
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA_RANDOM.End();
#endif
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