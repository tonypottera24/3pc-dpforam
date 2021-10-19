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

void Connection::WriteData(BinaryData &data, bool count_band) {
    // fprintf(stderr, "WriteData, size = %u\n", data.Size());
    Write(data.Dump(), data.Size(), count_band);
}

BinaryData &Connection::ReadData(const uint size) {
    // fprintf(stderr, "ReadData, size = %u\n", size);
    uchar binary_data[size];
    Read(binary_data, size);
    return *new BinaryData(binary_data, size);
}

void Connection::WriteData(std::vector<BinaryData> &data, bool count_band) {
    uint data_size = data[0].Size();
    uint buffer_size = data.size() * data_size;
    uchar buffer[buffer_size];
    for (uint i = 0; i < data.size(); i++) {
        memcpy(&buffer[i * data_size], data[i].Dump(), data_size);
    }
    Write(buffer, buffer_size, count_band);
}

std::vector<BinaryData> &Connection::ReadData(const uint size, const uint data_size) {
    uint buffer_size = size * data_size;
    uchar buffer[buffer_size];
    Read(buffer, buffer_size);
    std::vector<BinaryData> *data = new std::vector<BinaryData>;
    for (uint i = 0; i < size; i++) {
        data->emplace_back(&buffer[i * data_size], data_size);
    }
    return *data;
}