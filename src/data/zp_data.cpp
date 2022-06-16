#include "zp_data.h"

ZpData::ZpData() {
    this->data_ = BN_new();
    this->Reset();
}

ZpData::ZpData(const uint size) {
    this->data_ = BN_new();
    this->Reset();
}

ZpData::ZpData(const ZpData &other) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Start();
#endif
    this->data_ = BN_dup(other.data_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Stop();
#endif
}

ZpData::~ZpData() {
    BN_free(this->data_);
}

ZpData &ZpData::operator=(const ZpData &other) {
    if (this == &other) return *this;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Start();
#endif
    BN_copy(this->data_, other.data_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Stop();
#endif
    return *this;
}

ZpData ZpData::operator-() {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Start();
#endif
    BN_mod_sub(this->data_, this->p_, this->data_, this->p_, this->bn_ctx_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Stop();
#endif
    return *this;
}

ZpData &ZpData::operator+=(const ZpData &rhs) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Start();
#endif
    BN_mod_add(this->data_, this->data_, rhs.data_, this->p_, this->bn_ctx_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Stop();
#endif
    return *this;
}

ZpData &ZpData::operator-=(const ZpData &rhs) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Start();
#endif
    BN_mod_sub(this->data_, this->data_, rhs.data_, this->p_, this->bn_ctx_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Stop();
#endif
    return *this;
}

bool ZpData::operator==(const ZpData &rhs) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.compare_.Start();
#endif
    bool cmp = BN_cmp(this->data_, rhs.data_) == 0;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.compare_.Stop();
#endif
    return cmp;
}

void ZpData::DumpBuffer(uchar *buffer) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Start();
#endif
    BN_bn2binpad(this->data_, buffer, this->Size());
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Stop();
#endif
}

std::vector<uchar> ZpData::DumpVector() {
    std::vector<uchar> data(this->Size());
    DumpBuffer(data.data());
    return data;
}

void ZpData::LoadBuffer(uchar *buffer) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Start();
#endif
    BN_bin2bn(buffer, this->Size(), this->data_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Stop();
#endif
}

void ZpData::Reset() {
    BN_zero(this->data_);
}

void ZpData::Resize(const uint size) {}

void ZpData::Random(PRG *prg) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.random_.Start();
#endif
    if (prg == NULL) prg = this->prg_;
    prg->RandBn(this->data_, this->p_, this->bn_ctx_);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.random_.Stop();
#endif
}

void ZpData::Print(const char *title) {
#ifdef DEBUG
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%s\n", BN_bn2dec(this->data_));
#endif
}