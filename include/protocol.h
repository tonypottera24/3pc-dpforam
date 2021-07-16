#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include "connection.h"

class Protocol {
protected:
    Connection **cons;
    CryptoPP::AutoSeededRandomPool *rnd;
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs;

public:
    const char *party;

    Protocol(const char *party, Connection *cons[2],
             CryptoPP::AutoSeededRandomPool *rnd,
             CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs);
    virtual ~Protocol();
    void sync();
    unsigned long bandwidth();
    virtual void Test(uint iter) = 0;
};

#endif /* PROTOCOL_H_ */
