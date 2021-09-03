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

void Protocol::Sync() {
    uchar z = 0;
    this->conn_[0]->Write(&z, 1, false);
    this->conn_[1]->Write(&z, 1, false);
    this->conn_[0]->Read(&z, 1);
    this->conn_[1]->Read(&z, 1);
}

uint64_t Protocol::Bandwidth() {
    return this->conn_[0]->bandwidth_ + this->conn_[1]->bandwidth_;
}
