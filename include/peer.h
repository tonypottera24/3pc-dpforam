#ifndef PEER_H_
#define PEER_H_

#include "prg.h"
#include "socket.h"
#include "typedef.h"

class Peer {
private:
    Socket socket_;
    PRG prg_;

public:
    Peer() {}

    PRG *PRG();
    Socket &Socket();
    uint Bandwidth();

    void WriteUInt(uint n, bool count_band);
    uint ReadUInt();

    void WriteLong(uint64_t n, bool count_band);
    uint64_t ReadLong();

    template <typename D>
    D ReadData(const uint size) {
        // fprintf(stderr, "ReadData, size = %u\n", size);
        std::vector<uchar> buffer(size);
        this->socket_.Read(buffer.data(), size);
        D data;
        data.Load(buffer);
        return data;
    }

    template <typename D>
    void WriteData(D &data, bool count_band) {
        // fprintf(stderr, "WriteData, size = %u\n", data.Size());
        std::vector<uchar> dump = data.Dump();
        this->socket_.Write(dump.data(), dump.size(), count_band);
    }

    template <typename D>
    std::vector<D> ReadData(const uint size, const uint data_size) {
        uint buffer_size = size * data_size;
        uchar buffer[buffer_size];
        this->socket_.Read(buffer, buffer_size);
        std::vector<D> data(size);
        for (uint i = 0; i < size; i++) {
            std::vector<uchar> buf(buffer + data_size * i, buffer + data_size * (i + 1));
            data[i].Load(buf);
        }
        return data;
    }

    template <typename D>
    void WriteData(std::vector<D> &data, bool count_band) {
        std::vector<uchar> buffer;
        for (uint i = 0; i < data.size(); i++) {
            std::vector<uchar> dump = data[i].Dump();
            buffer.insert(buffer.end(), dump.begin(), dump.end());
        }
        this->socket_.Write(buffer.data(), buffer.size(), count_band);
    }

    void Close();
};

#endif /* PEER_H_ */
