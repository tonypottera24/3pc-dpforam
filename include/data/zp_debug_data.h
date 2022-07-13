#ifndef ZP_DEBUG_DATA_H_
#define ZP_DEBUG_DATA_H_

#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpDebugData {
private:
    uint data_ = 0;
    // const static uint p_ = 4294967291;
    const static uint p_ = 2147483647;
    static const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ZpDebugData();
    ZpDebugData(const uint size);
    ZpDebugData(const ZpDebugData &other);
    ~ZpDebugData();

    ZpDebugData &operator=(const ZpDebugData &other);
    ZpDebugData operator-();
    ZpDebugData &operator+=(const ZpDebugData &rhs);
    ZpDebugData &operator-=(const ZpDebugData &rhs);
    bool operator==(const ZpDebugData &rhs);

    friend ZpDebugData operator+(ZpDebugData lhs, const ZpDebugData &rhs) {
        lhs += rhs;
        return lhs;
    }

    friend ZpDebugData operator-(ZpDebugData lhs, const ZpDebugData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void DumpBuffer(uchar *buffer);
    std::vector<uchar> DumpVector();
    void LoadBuffer(uchar *buffer);

    uint64_t hash(uint64_t digest_n, uint round);

    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);

    uint Size() { return sizeof(uint); }
    static bool IsSymmetric() { return is_symmetric_; }

    void Print(const char *title = "");
};

#endif /* ZP_DEBUG_DATA_H_ */