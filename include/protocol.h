#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include "connection.h"

class Protocol {
protected:
    Connection **conn_;
    CryptoPP::AutoSeededRandomPool *rnd_;
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs_;

public:
    const char *kParty;

    Protocol(const char *party, Connection *cons[2],
             CryptoPP::AutoSeededRandomPool *rnd,
             CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs);
    virtual ~Protocol();
    void Sync();
    unsigned long Bandwidth();
    virtual void Test(uint iter) = 0;
};

#endif /* PROTOCOL_H_ */
