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
    void P0(const uint64_t b0, uchar** u, const uint64_t n, const uint64_t data_size, uchar* p0, bool count_band = true);
    void P1(const uint64_t b1, uchar** v, const uint64_t n, const uint64_t data_size, uchar* p1, bool count_band = true);
    void Test(uint iter);
};

#endif /* SSOT_H_ */