#ifndef DPF_KEY_H
#define DPF_KEY_H

#include <vector>

#include "block.h"

template <typename D>
class DPFKey {
private:
public:
    uint128 s0;
    bool t0;
    std::vector<uint128> sCW;
    std::vector<bool> tCW[2];
    D last_CW;
};
#endif