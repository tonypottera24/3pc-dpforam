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
    this->data_ = BN_dup(other.data_);
}

ZpData::~ZpData() {
    BN_free(this->data_);
}

ZpData &ZpData::operator=(const ZpData &other) {
    // copy operation
    if (this == &other) return *this;
    BN_copy(this->data_, other.data_);
    return *this;
}

ZpData ZpData::operator-() {
    BN_mod_inverse(this->data_, this->data_, this->p_, this->bn_ctx_);
    return *this;
}

void ZpData::Add(const ZpData &a, const ZpData &b, ZpData &r) {
    BN_mod_add(r.data_, a.data_, b.data_, ZpData::p_, ZpData::bn_ctx_);
}

void ZpData::Minus(const ZpData &a, const ZpData &b, ZpData &r) {
    BN_mod_sub(r.data_, a.data_, b.data_, ZpData::p_, ZpData::bn_ctx_);
}

bool ZpData::operator==(const ZpData &rhs) {
    return BN_cmp(this->data_, rhs.data_) == 0;
}

void ZpData::Dump(std::vector<uchar> &data) {
    uint size = BN_num_bytes(this->data_);
    data.resize(size);
    BN_bn2bin(this->data_, data.data());
}

void ZpData::Load(std::vector<uchar> &data) {
    BN_bin2bn(data.data(), data.size(), this->data_);
}

void ZpData::Reset() {
    BN_zero_ex(this->data_);
}

void ZpData::Resize(const uint size) {
}

void ZpData::Random(PRG *prg) {
    if (prg == NULL) prg = this->prg_;
    prg->RandBn(this->data_, this->p_);
}

void ZpData::Print(const char *title) {
#ifdef DEBUG
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    debug_print("%s\n", BN_bn2dec(this->data_));
#endif
}