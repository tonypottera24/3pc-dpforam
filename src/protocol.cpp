#include "protocol.h"

Protocol::Protocol(const char *party, Connection *connections[2],
                   CryptoPP::AutoSeededRandomPool *rnd,
                   CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs) {
    this->kParty = party;
    this->connnections_ = connections;
    this->rnd_ = rnd;
    this->prgs_ = prgs;
}

Protocol::~Protocol() {
}

void Protocol::Sync() {
    uchar z = 0;
    this->connnections_[0]->Write(&z, 1, false);
    this->connnections_[1]->Write(&z, 1, false);
    this->connnections_[0]->Read(&z, 1);
    this->connnections_[1]->Read(&z, 1);
}

unsigned long Protocol::Bandwidth() {
    return this->connnections_[0]->bandwidth_ + this->connnections_[1]->bandwidth_;
}
