#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include "connection.h"

class Protocol {
protected:
    uint party_;
    Connection **conn_;
    CryptoPP::AutoSeededRandomPool *rnd_;
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs_;

public:
    Protocol(const uint party, Connection *cons[2],
             CryptoPP::AutoSeededRandomPool *rnd,
             CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs);
    virtual ~Protocol();
    void Sync();
    uint64_t Bandwidth();
    virtual void Test(uint iter) = 0;
};

#endif /* PROTOCOL_H_ */
