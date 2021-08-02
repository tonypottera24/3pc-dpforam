#include "connection.h"

Connection::~Connection() {
}

void Connection::WriteInt(int n, bool count_band) {
    Write((uchar*)&n, 4, count_band);
}

int Connection::ReadInt() {
    int n;
    Read((uchar*)&n, 4);
    return n;
}

void Connection::WriteLong(uint64_t n, bool count_band) {
    Write((uchar*)&n, 8, count_band);
}

uint64_t Connection::ReadLong() {
    uint64_t n;
    Read((uchar*)&n, 8);
    return n;
}
