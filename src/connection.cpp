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

void Connection::WriteLong(long n, bool count_band) {
    Write((uchar*)&n, 8, count_band);
}

long Connection::ReadLong() {
    long n;
    Read((uchar*)&n, 8);
    return n;
}
