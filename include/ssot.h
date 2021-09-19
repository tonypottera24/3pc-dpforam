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
    void P1(const uint b1, const uchar* v01[2], const uint data_size, uchar* p1);
    void P2(const uint data_size);
    void P0(const uint b0, const uchar* u01[2], const uint data_size, uchar* p0);
    void Test(uint iter);
};

#endif /* SSOT_H_ */