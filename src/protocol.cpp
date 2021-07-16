#include "protocol.h"

Protocol::Protocol(const char *party, Connection *cons[2],
                   CryptoPP::AutoSeededRandomPool *rnd,
                   CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs) {
    this->party = party;
    this->cons = cons;
    this->rnd = rnd;
    this->prgs = prgs;
}

Protocol::~Protocol() {
}

void Protocol::sync() {
    uchar z = 0;
    cons[0]->write(&z, 1, false);
    cons[1]->write(&z, 1, false);
    cons[0]->read(&z, 1);
    cons[1]->read(&z, 1);
}

unsigned long Protocol::bandwidth() {
    return cons[0]->bandwidth + cons[1]->bandwidth;
}
