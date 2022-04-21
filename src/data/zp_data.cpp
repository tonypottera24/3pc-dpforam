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

ZpData &ZpData::operator+=(const ZpData &rhs) {
    BN_mod_add(this->data_, this->data_, rhs.data_, this->p_, this->bn_ctx_);
    return *this;
}

ZpData &ZpData::operator-=(const ZpData &rhs) {
    BN_mod_sub(this->data_, this->data_, rhs.data_, this->p_, this->bn_ctx_);
    return *this;
}

bool ZpData::operator==(const ZpData &rhs) {
    return BN_cmp(this->data_, rhs.data_) == 0;
}

std::vector<uchar> ZpData::Dump() {
    std::vector<uchar> data(this->Size());
    BN_bn2binpad(this->data_, data.data(), this->Size());
    return data;
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