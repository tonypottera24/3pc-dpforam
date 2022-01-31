
#include "peer.h"

Socket &Peer::Socket() {
    return this->socket_;
}

PRG &Peer::PRG() {
    return this->prg_;
}

uint Peer::Bandwidth() {
    return this->socket_.Bandwidth();
}

void Peer::WriteUInt(uint n, bool count_band) {
    this->socket_.Write((uchar *)&n, sizeof(uint), count_band);
}

uint Peer::ReadUInt() {
    uint n;
    this->socket_.Read((uchar *)&n, sizeof(uint));
    return n;
}

void Peer::WriteLong(uint64_t n, bool count_band) {
    this->socket_.Write((uchar *)&n, sizeof(uint64_t), count_band);
}

uint64_t Peer::ReadLong() {
    uint64_t n;
    this->socket_.Read((uchar *)&n, sizeof(uint64_t));
    return n;
}

void Peer::Close() {
    this->socket_.Close();
}