#ifndef SSOT_H_
#define SSOT_H_

#include <iostream>

#include "protocol.h"
#include "util.h"

class SSOT : public Protocol {
private:
    const DataType data_type_;

public:
    SSOT(const uint party, const DataType data_type, Connection* cons[2],
         CryptoPP::AutoSeededRandomPool* rnd,
         CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption* prgs);
    void P2(const uint64_t n, const uint64_t data_size, bool count_band = true);
    Data* P0(const uint64_t b0, std::vector<Data>& u, bool count_band = true);
    Data* P1(const uint64_t b1, std::vector<Data>& v, bool count_band = true);
    void Test(uint iter);
};

#endif /* SSOT_H_ */