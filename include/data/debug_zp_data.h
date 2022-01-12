#ifndef ZP_DATA_H_
#define ZP_DATA_H_

#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <inttypes.h>

#include "typedef.h"
#include "util.h"

class DebugZpData {
private:
    uint64_t data_ = 0;
    const uint64_t p_ = 11;
    const bool is_symmetric_ = false;

public:
    DebugZpData();
    DebugZpData(const DebugZpData &other);
    DebugZpData(uchar *data, const uint size);
    DebugZpData(const uint size, const bool set_zero = false);
    ~DebugZpData();

    DebugZpData &operator=(const DebugZpData &other);
    // DebugZpData &operator=(DebugZpData &&other) noexcept;
    DebugZpData operator-();
    DebugZpData &operator+=(const DebugZpData &rhs);
    DebugZpData &operator-=(const DebugZpData &rhs);
    bool operator==(const DebugZpData &rhs);
    friend DebugZpData operator+(DebugZpData lhs, const DebugZpData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend DebugZpData operator-(DebugZpData lhs, const DebugZpData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    uchar *Dump();
    void Load(uchar *data, uint size);
    void Reset();
    void Random(uint size);
    void Random(CryptoPP::RandomNumberGenerator &prg, uint size);
    uint Size() { return sizeof(uint64_t); }
    bool IsSymmetric() { return this->is_symmetric_; }
    void Print(const char *title = "");
};

#endif /* ZP_DATA_H_ */