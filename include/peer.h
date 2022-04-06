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

    void WriteUint64(uint64_t n, bool count_band);
    uint64_t ReadUint64();

    template <typename D>
    void ReadData(D &data) {
        // fprintf(stderr, "ReadData, size = %u\n", size);
        uint data_size = data.Size();
        std::vector<uchar> buffer(data_size);
        this->socket_.Read(buffer.data(), data_size);
        data.Load(buffer);
    }

    template <typename D>
    void ReadDataVector(std::vector<D> &data) {
        uint size = data.size();
        uint data_size = data[0].Size();
        uint buffer_size = size * data_size;
        uchar buffer[buffer_size];
        this->socket_.Read(buffer, buffer_size);
        for (uint i = 0; i < size; i++) {
            std::vector<uchar> buf(buffer + data_size * i, buffer + data_size * (i + 1));
            // TODO turn to array design?
            data[i].Load(buf);
        }
    }

    template <typename D>
    void WriteDataVector(std::vector<D> &data, bool count_band) {
        std::vector<uchar> buffer;
        std::vector<uchar> dump;
        for (uint i = 0; i < data.size(); i++) {
            dump = data[i].Dump();
            buffer.insert(buffer.end(), dump.begin(), dump.end());
        }
        this->socket_.Write(buffer.data(), buffer.size(), count_band);
    }

    template <typename D>
    void WriteData(D &data, bool count_band) {
        // fprintf(stderr, "WriteData, size = %u\n", data.Size());
        const std::vector<uchar> dump = data.Dump();
        this->socket_.Write(dump.data(), dump.size(), count_band);
    }

    void Close();
};

#endif /* PEER_H_ */
