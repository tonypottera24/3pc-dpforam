
#include "peer.h"

Peer::Peer(class Socket socket, CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption prg) : socket_(socket), prg_(prg) {
    // fprintf(stderr, "Initilizing server %s:%u...\n", ip.c_str(), port);
}

Socket &Peer::Socket() {
    return this->socket_;
}

CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &Peer::PRG() {
    return this->prg_;
}

uint64_t Peer::Bandwidth() {
    return this->bandwidth_;
}

void Peer::WriteLong(uint64_t n, bool count_band) {
    this->socket_.Write((uchar *)&n, 8, count_band);
}

uint64_t Peer::ReadLong() {
    uint64_t n;
    this->socket_.Read((uchar *)&n, 8);
    return n;
}

void Peer::Close() {
    this->socket_.Close();
}