#include "zp_boost_data.h"

ZpBoostData::ZpBoostData() {
}

ZpBoostData::ZpBoostData(const uint size) {
}

ZpBoostData::ZpBoostData(const ZpBoostData &other) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Start();
#endif
    this->data_ = other.data_;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Stop();
#endif
}

ZpBoostData::~ZpBoostData() {
}

ZpBoostData &ZpBoostData::operator=(const ZpBoostData &other) {
    if (this == &other) return *this;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Start();
#endif
    this->data_ = other.data_;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.copy_.Stop();
#endif
    return *this;
}

ZpBoostData ZpBoostData::operator-() {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Start();
#endif
    this->data_ = this->p_ - this->data_;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Stop();
#endif
    return *this;
}

ZpBoostData &ZpBoostData::operator+=(const ZpBoostData &rhs) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Start();
#endif
    // this->data_ = (this->data_ + rhs.data_) % this->p_;
    if (this->data_ >= this->p_ - rhs.data_) {
        this->data_ += rhs.data_;
        this->data_ -= this->p_;
    } else {
        this->data_ += rhs.data_;
    }
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Stop();
#endif
    return *this;
}

ZpBoostData &ZpBoostData::operator-=(const ZpBoostData &rhs) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Start();
#endif
    // this->data_ = (this->data_ + (this->p_ - rhs.data_)) % this->p_;
    uint256_t tmp = this->p_ - rhs.data_;

    if (this->data_ >= this->p_ - tmp) {
        this->data_ += tmp;
        this->data_ -= this->p_;
    } else {
        this->data_ += tmp;
    }
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.arithmatic_.Stop();
#endif
    return *this;
}

bool ZpBoostData::operator==(const ZpBoostData &rhs) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.compare_.Start();
#endif
    bool cmp = this->data_ == rhs.data_;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.compare_.Stop();
#endif
    return cmp;
}

void ZpBoostData::DumpBuffer(uchar *buffer) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Start();
#endif
    // std::cout << "this->data_ " << this->data_ << std::endl;

    export_bits(this->data_, buffer, 8, false);
    // print_bytes(buffer, this->Size(), "exported buffer");

    // std::vector<unsigned char> v;
    // export_bits(this->data_, std::back_inserter(v), 8, false);
    // print_bytes(v.data(), v.size(), "v");
    // debug_print("v.size = %zu\n", v.size());
    // fprintf(stderr, "v.size = %zu\n", v.size());

    // uint512_t v_tmp;
    // import_bits(v_tmp, v.begin(), v.end());
    // import_bits(v_tmp, v.begin(), v.end(), 8, false);
    // std::cout << "v_tmp = " << v_tmp << std::endl;
    // assert(v_tmp == this->data_);

    // uint512_t b_tmp;
    // import_bits(b_tmp, buffer, buffer + this->Size(), 8, false);
    // std::cout << "b_tmp = " << b_tmp << std::endl;
    // assert(b_tmp == this->data_);

    // memcpy(buffer, v.data(), v.size());
    // assert(v.size() == 32 || v.size() == 1);
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Stop();
#endif
}

std::vector<uchar> ZpBoostData::DumpVector() {
    std::vector<uchar> data(this->Size());
    DumpBuffer(data.data());
    return data;
}

void ZpBoostData::LoadBuffer(uchar *buffer) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Start();
#endif
    import_bits(this->data_, buffer, buffer + this->Size(), 8, false);
    // memcpy(buffer, this->data_.backend().limbs(), );
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.dump_load_.Stop();
#endif
}

uint64_t ZpBoostData::hash(uint64_t digest_n, uint64_t round) {
    // return this->boost_hash(this->data_ * uint256_t(round + 1)) % digest_n;

    // return (uint64_t)(this->data_ * uint256_t(round + 1)) % digest_n;
    // initAESKey(round);
    uint256_t data = this->data_;
    // uint128 stash[2];
    // DumpBuffer((uchar *)stash);
    AES_ecb_encrypt_blks((uint128 *)&data, 2, &(ZpBoostData::aes_key_[round]));
    // import_bits(data, (uchar *)stash, (uchar *)stash + this->Size(), 8, false);

    return (uint64_t)data % digest_n;
}

void ZpBoostData::Reset() {
    this->data_ = 0;
}

void ZpBoostData::Resize(const uint size) {}

void ZpBoostData::Random(PRG *prg) {
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.random_.Start();
#endif
    if (prg == NULL) prg = this->prg_;
    uchar buffer[this->Size()];
    prg->RandBytes(buffer, this->Size());
    import_bits(this->data_, buffer, buffer + this->Size(), 8, false);
    this->data_ %= this->p_;
#ifdef BENCHMARK_ZP_DATA
    Benchmark::ZP_DATA.random_.Stop();
#endif
}

void ZpBoostData::Print(const char *title) {
#ifdef DEBUG
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    std::cout << this->data_ << std::endl;
#endif
}