#include "ec_data.h"

ECData::ECData() {
    this->data_ = EC_POINT_new(this->curve_);
    EC_POINT_set_to_infinity(this->curve_, this->data_);
    // this->data_ = EC_POINT_dup(this->g_, this->curve_);
    // EC_POINT_invert(this->curve_, this->data_, this->bn_ctx_);
    // EC_POINT_add(this->curve_, this->data_, this->data_, this->g_, this->bn_ctx_);
}

ECData::ECData(uchar *data, const uint size) {
    this->data_ = EC_POINT_new(this->curve_);
    EC_POINT_oct2point(this->curve_, this->data_, data, size, this->bn_ctx_);
}

ECData::ECData(const uint size, const bool set_zero) {
    this->data_ = EC_POINT_new(this->curve_);
    EC_POINT_set_to_infinity(this->curve_, this->data_);
    // this->data_ = EC_POINT_dup(this->g_, this->curve_);
    // EC_POINT_invert(this->curve_, this->data_, this->bn_ctx_);
    // EC_POINT_add(this->curve_, this->data_, this->data_, this->g_, this->bn_ctx_);
}

ECData::ECData(const ECData &other) {
    this->data_ = EC_POINT_dup(other.data_, this->curve_);
}

ECData::~ECData() {
    // BN_CTX_free(bn_ctx_);
    EC_POINT_free(this->data_);
}

ECData &ECData::operator=(const ECData &other) {
    // copy operation
    if (this == &other) return *this;
    EC_POINT_copy(this->data_, other.data_);
    return *this;
}

ECData ECData::operator-() {
    EC_POINT_invert(this->curve_, this->data_, this->bn_ctx_);
    return *this;
}

ECData &ECData::operator+=(const ECData &rhs) {
    EC_POINT_add(this->curve_, this->data_, this->data_, rhs.data_, this->bn_ctx_);
    return *this;
}

ECData &ECData::operator-=(const ECData &rhs) {
    EC_POINT_invert(this->curve_, this->data_, this->bn_ctx_);
    EC_POINT_add(this->curve_, this->data_, this->data_, rhs.data_, this->bn_ctx_);
    EC_POINT_invert(this->curve_, this->data_, this->bn_ctx_);
    return *this;
}

bool ECData::operator==(const ECData &rhs) {
    return EC_POINT_cmp(this->curve_, this->data_, rhs.data_, this->bn_ctx_) == 0;
}

void ECData::Dump(uchar *data) {
    EC_POINT_point2oct(this->curve_, this->data_, POINT_CONVERSION_COMPRESSED, data, this->Size(), this->bn_ctx_);
}

void ECData::Load(uchar *data, uint size) {
    EC_POINT_oct2point(this->curve_, this->data_, data, size, this->bn_ctx_);
}

void ECData::Reset() {
    // this->data_ = this->curve_.Identity();
}

void ECData::Random(uint size) {
    PRG prg;
    this->Random(prg, size);
}

void ECData::Random(PRG &prg, uint size) {
    // debug_print("ECData::Random start\n");
    // BN_CTX_start(this->bn_ctx_);
    // BIGNUM *x = BN_CTX_get(this->bn_ctx_);
    // BIGNUM *q = BN_CTX_get(this->bn_ctx_);
    // EC_GROUP_get_order(this->curve_, q, this->bn_ctx_);
    // prg.RandBn(x, q);
    // const EC_POINT *g = EC_GROUP_get0_generator(this->curve_);
    // EC_POINT_copy(this->data_, g);
    // EC_POINT_mul(this->curve_, this->data_, x, NULL, NULL, this->bn_ctx_);
    // BN_CTX_end(this->bn_ctx_);
    // debug_print("ECData::Random end\n");

    const EC_POINT *g = EC_GROUP_get0_generator(this->curve_);
    EC_POINT_copy(this->data_, g);

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

void ECData::Print(const char *title) {
    if (strlen(title) > 0) {
        debug_print("%s ", title);
    }
    uchar s[this->Size()];
    this->Dump(s);
    print_bytes(s, this->Size(), "");

    // debug_print("point2bn: ");
    BIGNUM *x = BN_new();
    BIGNUM *y = BN_new();
    EC_POINT_get_affine_coordinates(this->curve_, this->data_, x, y, this->bn_ctx_);
    char *hex_x = BN_bn2hex(x);
    char *hex_y = BN_bn2hex(y);
    debug_print("x = %s\n", hex_x);
    debug_print("y = %s\n", hex_y);
}