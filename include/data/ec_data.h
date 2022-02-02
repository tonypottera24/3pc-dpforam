#ifndef EC_DATA_H_
#define EC_DATA_H_

#include <inttypes.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <iostream>

#include "typedef.h"
#include "util.h"

class ECData {
private:
    EC_POINT *data_;
    const static inline EC_GROUP *curve_ = EC_GROUP_new_by_curve_name(NID_secp256k1);
    const static inline EC_POINT *g_ = EC_GROUP_get0_generator(EC_GROUP_new_by_curve_name(NID_secp256k1));
    const static inline uint size_ = 33;
    static inline BN_CTX *bn_ctx_ = BN_CTX_new();
    const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ECData();
    ECData(const ECData &other);
    // ECData(uchar *data, const uint size);
    ECData(const uint size);
    ~ECData();

    ECData &operator=(const ECData &other);
    // ECData &operator=(ECData &&other) noexcept;
    ECData operator-();
    ECData &operator+=(const ECData &rhs);
    ECData &operator-=(const ECData &rhs);
    bool operator==(const ECData &rhs);
    friend ECData operator+(ECData lhs, const ECData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend ECData operator-(ECData lhs, const ECData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    std::vector<uchar> Dump();
    void Load(std::vector<uchar> data);
    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);
    uint Size() {
        // uint size = EC_POINT_point2oct(this->curve_, this->data_, POINT_CONVERSION_COMPRESSED, NULL, 0, this->bn_ctx_);
        // fprintf(stderr, "size = %u\n", size);
        return this->size_;
    }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* EC_DATA_H_ */