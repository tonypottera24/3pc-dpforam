#ifndef ZP_DEBUG_DATA_H_
#define ZP_DEBUG_DATA_H_

#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class ZpDebugData {
public:
    uint data_ = 0;
    const static uint p_ = 11;
    const bool is_symmetric_ = false;
    static inline PRG *prg_ = new PRG();

public:
    ZpDebugData();
    ZpDebugData(const ZpDebugData &other);
    ZpDebugData(const uint size);
    ~ZpDebugData();

    ZpDebugData &operator=(const ZpDebugData &other);
    ZpDebugData operator-();
    bool operator==(const ZpDebugData &rhs);

    static void Add(const ZpDebugData &a, const ZpDebugData &b, ZpDebugData &r);
    static void Minus(const ZpDebugData &a, const ZpDebugData &b, ZpDebugData &r);

    void Dump(std::vector<uchar> &data);
    void Load(std::vector<uchar> &data);
    void Reset();
    void Resize(const uint size);
    void Random(PRG *prg = NULL);
    uint Size() { return sizeof(uint); }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* ZP_DEBUG_DATA_H_ */