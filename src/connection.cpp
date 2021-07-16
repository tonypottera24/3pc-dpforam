#include "connection.h"

Connection::~Connection() {
}

void Connection::write_int(int n, bool count_band) {
    write((uchar*)&n, 4, count_band);
}

int Connection::read_int() {
    int n;
    read((uchar*)&n, 4);
    return n;
}

void Connection::write_long(long n, bool count_band) {
    write((uchar*)&n, 8, count_band);
}

long Connection::read_long() {
    long n;
    read((uchar*)&n, 8);
    return n;
}
