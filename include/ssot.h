#ifndef SSOT_H_
#define SSOT_H_

#include <iostream>

#include "protocol.h"
#include "util.h"

class SSOT : public Protocol {
public:
    SSOT(const uint party, Connection* cons[2],
         CryptoPP::AutoSeededRandomPool* rnd,
         CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption* prgs);
    void P2(const uint64_t n, const uint64_t data_size, bool count_band = true);
    BinaryData* P0(const uint64_t b0, std::vector<BinaryData>& u, bool count_band = true);
    BinaryData* P1(const uint64_t b1, std::vector<BinaryData>& v, bool count_band = true);
    void Test(uint iter);
};

#endif /* SSOT_H_ */