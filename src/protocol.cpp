#include "protocol.h"

Protocol::Protocol(const uint party, Connection *connections[2],
                   CryptoPP::AutoSeededRandomPool *rnd,
                   CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs) {
    this->party_ = party;
    this->conn_ = connections;
    this->rnd_ = rnd;
    this->prgs_ = prgs;
}

Protocol::~Protocol() {
}

uint64_t Protocol::Bandwidth() {
    return this->conn_[0]->bandwidth_ + this->conn_[1]->bandwidth_;
}
