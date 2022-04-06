
#include "peer.h"

Socket &Peer::Socket() {
    return this->socket_;
}

PRG *Peer::PRG() {
    return &this->prg_;
}

void Peer::WriteUInt(uint n, Benchmark::Record *benchmark) {
    this->socket_.Write((uchar *)&n, sizeof(uint), benchmark);
}

uint Peer::ReadUInt() {
    uint n;
    this->socket_.Read((uchar *)&n, sizeof(uint));
    return n;
}

void Peer::WriteUInt64(uint64_t n, Benchmark::Record *benchmark) {
    this->socket_.Write((uchar *)&n, sizeof(uint64_t), benchmark);
}

uint64_t Peer::ReadUInt64() {
    uint64_t n;
    this->socket_.Read((uchar *)&n, sizeof(uint64_t));
    return n;
}

void Peer::Close() {
    this->socket_.Close();
}