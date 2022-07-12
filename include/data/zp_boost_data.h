#ifndef ZP_BOOST_DATA_H_
#define ZP_BOOST_DATA_H_

#include <inttypes.h>

#include <boost/multiprecision/cpp_int.hpp>

#include "benchmark/constant.h"
#include "typedef.h"
#include "util.h"

using namespace boost::multiprecision;

class ZpBoostData {
private:
    uint512_t data_ = uint512_t(0);
    static const inline uint512_t p_ = uint512_t("0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff");
    // static const inline uint512_t p_ = uint512_t(11);
    // const static inline uint size_ = uint512_t("0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff").backend().size();
    // const static inline uint size_ = sizeof(uint512_t);
    const static inline uint size_ = 32;
    static const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ZpBoostData();
    ZpBoostData(const uint size);
    ZpBoostData(const ZpBoostData &other);
    ~ZpBoostData();

    ZpBoostData &operator=(const ZpBoostData &other);
    ZpBoostData &operator+=(const ZpBoostData &rhs);
    ZpBoostData &operator-=(const ZpBoostData &rhs);
    bool operator==(const ZpBoostData &rhs);

    ZpBoostData operator-();
    friend ZpBoostData operator+(ZpBoostData lhs, const ZpBoostData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend ZpBoostData operator-(ZpBoostData lhs, const ZpBoostData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void DumpBuffer(uchar *buffer);
    std::vector<uchar> DumpVector();
    void LoadBuffer(uchar *buffer);

    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);

    uint Size() { return this->size_; }
    static bool IsSymmetric() { return is_symmetric_; }

    void Print(const char *title = "");
};

#endif /* ZP_BOOST_DATA_H_ */