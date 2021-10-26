#include "connection.h"

Connection::~Connection() {
}

void Connection::WriteLong(uint64_t n, bool count_band) {
    Write((uchar *)&n, 8, count_band);
}

uint64_t Connection::ReadLong() {
    uint64_t n;
    Read((uchar *)&n, 8);
    return n;
}