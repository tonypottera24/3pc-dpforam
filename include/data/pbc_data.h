// #ifndef PBC_DATA_H_
// #define PBC_DATA_H_

// #include <inttypes.h>
// #include <openssl/ec.h>
// #include <openssl/obj_mac.h>
// #include <pbc/pbc.h>

// #include <iostream>

// #include "typedef.h"
// #include "util.h"

// class PBCData {
// private:
//     element_s *data_;
//     const static inline EC_GROUP *curve_ = EC_GROUP_new_by_curve_name(NID_secp256k1);
//     const static inline EC_POINT *g_ = EC_GROUP_get0_generator(EC_GROUP_new_by_curve_name(NID_secp256k1));
//     const static inline uint size_ = 65;
//     // const static inline uint size_ = 33;
//     static inline BN_CTX *bn_ctx_ = BN_CTX_new();
//     const bool is_symmetric_ = false;
//     static inline PRG *prg_ = new PRG();

// public:
//     PBCData();
//     PBCData(const PBCData &other);
//     PBCData(const uint size);
//     ~PBCData();

//     PBCData &operator=(const PBCData &other);
//     PBCData operator-();
//     PBCData &operator+=(const PBCData &rhs);
//     PBCData &operator-=(const PBCData &rhs);
//     bool operator==(const PBCData &rhs);
//     friend PBCData operator+(PBCData lhs, const PBCData &rhs) {
//         lhs += rhs;
//         return lhs;
//     }
//     friend PBCData operator-(PBCData lhs, const PBCData &rhs) {
//         lhs -= rhs;
//         return lhs;
//     }

//     void Dump(std::vector<uchar> &data);
//     void Load(std::vector<uchar> &data);
//     void Reset();
//     void Resize(const uint size);
//     void Random(PRG *prg = NULL);
//     uint Size() {
//         // uint size = EC_POINT_point2oct(this->curve_, this->data_, POINT_CONVERSION_COMPRESSED, NULL, 0, this->bn_ctx_);
//         // fprintf(stderr, "size = %u\n", size);
//         return this->size_;
//     }
//     bool IsSymmetric() { return this->is_symmetric_; }
//     void Print(const char *title = "");
// };

// #endif /* PBC_DATA_H_ */