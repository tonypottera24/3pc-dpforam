#include "data/binary_data.h"

BinaryData::BinaryData() {
}

BinaryData::BinaryData(const uint size) {
    this->Resize(size);
    this->Reset();
}

BinaryData::BinaryData(const BinaryData &other) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.copy_.Start();
#endif
    this->data_ = other.data_;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.copy_.Stop();
#endif
}

BinaryData::~BinaryData() {
    // this->data_.clear();
}

BinaryData &BinaryData::operator=(const BinaryData &other) {
    // copy operation
    if (this == &other) return *this;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.copy_.Start();
#endif
    this->data_ = other.data_;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.copy_.Stop();
#endif
    return *this;
}

BinaryData BinaryData::operator-() {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.arithmatic_.Start();
#endif
    neg_bytes(this->data_.data(), this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.arithmatic_.Stop();
#endif
    return *this;
}

BinaryData &BinaryData::operator+=(const BinaryData &rhs) {
    // assert(this->data_.size() == rhs.data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.arithmatic_.Start();
#endif
    xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.arithmatic_.Stop();
#endif
    return *this;
}

BinaryData &BinaryData::operator-=(const BinaryData &rhs) {
    // assert(this->data_.size() == rhs.data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.arithmatic_.Start();
#endif
    xor_bytes(this->data_.data(), rhs.data_.data(), this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.arithmatic_.Stop();
#endif
    return *this;
}

bool BinaryData::operator==(const BinaryData &rhs) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.compare_.Start();
#endif
    bool cmp = this->data_.size() == rhs.data_.size() &&
               memcmp(this->data_.data(), rhs.data_.data(), this->data_.size()) == 0;
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.compare_.Stop();
#endif
    return cmp;
}

void BinaryData::DumpBuffer(uchar *buffer) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.dump_load_.Start();
#endif
    memcpy(buffer, this->data_.data(), this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.dump_load_.Stop();
#endif
}

std::vector<uchar> BinaryData::DumpVector() {
    return this->data_;
}

void BinaryData::LoadBuffer(uchar *buffer) {
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.dump_load_.Start();
#endif
    memcpy(this->data_.data(), buffer, this->data_.size());
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.dump_load_.Stop();
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
    Benchmark::BINARY_DATA.random_.Start();
#endif
    if (prg == NULL) prg = this->prg_;
    if (this->data_.size() > 0) {
        prg->RandBytes(this->data_.data(), this->data_.size());
    }
#ifdef BENCHMARK_BINARY_DATA
    Benchmark::BINARY_DATA.random_.Stop();
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