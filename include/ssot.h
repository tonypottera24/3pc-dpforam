#ifndef SSOT_H_
#define SSOT_H_

#include <iostream>

#include "protocol.h"
#include "util.h"

class SSOT : public Protocol {
public:
    SSOT(const char* party, Connection* cons[2],
         CryptoPP::AutoSeededRandomPool* rnd,
         CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption* prgs);
    void P2(uint b1, const uchar* const v01[2], uint data_size, uchar* p1);
    void P3(uint data_size);
    void P1(uint b0, const uchar* const u01[2], uint data_size, uchar* p0);
    void Test(uint iter);
};

#endif /* SSOT_H_ */