#include "pbc_data.h"

PBCData::PBCData() {
    element_init_G1(this->data_, );
    this->Reset();
}

PBCData::PBCData(const uint size) {
    element_init_G1(this->data_, );
    this->Reset();
}

PBCData::PBCData(const PBCData &other) {
    element_set(other.data_, this->data_);
}

PBCData::~PBCData() {
    element_clear(this->data_);
}

PBCData &PBCData::operator=(const PBCData &other) {
    // copy operation
    if (this == &other) return *this;
    element_set(other.data_, this->data_);
    return *this;
}

PBCData PBCData::operator-() {
    element_neg(this->data_, this->data_);
    return *this;
}

PBCData &PBCData::operator+=(const PBCData &rhs) {
    element_add(this->data_, this->data_, rhs.data_);
    return *this;
}

PBCData &PBCData::operator-=(const PBCData &rhs) {
    element_sub(this->data_, this->data_, rhs.data_);
    return *this;
}

bool PBCData::operator==(const PBCData &rhs) {
    return element_cmp(this->data_, rhs.data_) == 0;
}

std::vector<uchar> PBCData::Dump() {
    std::vector<uchar> data(this->Size());
    EC_POINT_point2oct(this->curve_, this->data_, POINT_CONVERSION_UNCOMPRESSED, data.data(), data.size(), this->bn_ctx_);
    return data;
}

void PBCData::Load(std::vector<uchar> data) {
    EC_POINT_oct2point(this->curve_, this->data_, data.data(), data.size(), this->bn_ctx_);
}

void PBCData::Reset() {
    element_set0(this->data_);
}

void PBCData::Resize(const uint size) {
}

void PBCData::Random(PRG *prg) {
    if (prg == NULL) prg = this->prg_;
    BN_CTX_start(this->bn_ctx_);
    BIGNUM *x = BN_CTX_get(this->bn_ctx_);
    BIGNUM *q = BN_CTX_get(this->bn_ctx_);
    EC_GROUP_get_order(this->curve_, q, this->bn_ctx_);
    prg->RandBn(x, q);
    EC_POINT_copy(this->data_, this->g_);
    EC_POINT_mul(this->curve_, this->data_, x, NULL, NULL, this->bn_ctx_);
    BN_CTX_end(this->bn_ctx_);

    // const EC_POINT *g = EC_GROUP_get0_generator(this->curve_);
    // EC_POINT_copy(this->data_, g);

    // bool v = x.GetBit(0);
    // while (true) {
    //     Integer r = ((a_exp_b_mod_c(x, Integer("3"), this->p_) + (this->a_ * x) % this->p_) % this->p_ + this->b_) % this->p_;
    //     Integer y = a_exp_b_mod_c(r, this->q_, this->p_);
    //     if (a_exp_b_mod_c(y, Integer::Two(), this->p_) == r) {
    //         if (v) {
    //             this->data_ = ECP::Point(x, y);
    //         } else {
    //             this->data_ = ECP::Point(x, (this->p_ - y) % this->p_);
    //         }
    //         break;
    //     }
    //     x += 1;
    // }
}

void PBCData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    // std::vector<uchar> s = this->Dump();
    // print_bytes(s.data(), s.size(), "");

    // BIGNUM *x = BN_new();
    // BIGNUM *y = BN_new();
    // EC_POINT_get_affine_coordinates(this->curve_, this->data_, x, y, this->bn_ctx_);
    // debug_print("x = %s\n", BN_bn2hex(x));
    // debug_print("y = %s\n", BN_bn2hex(y));
}