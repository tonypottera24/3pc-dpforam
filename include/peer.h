#ifndef PEER_H_
#define PEER_H_

#include "prg.h"
#include "socket.h"
#include "typedef.h"
namespace Benchmark {
class Record;
};
class Peer {
private:
    Socket socket_;
    PRG prg_;

public:
    Peer() {}

    Socket &Socket() {
        return this->socket_;
    }

    PRG *PRG() {
        return &this->prg_;
    }

    void WriteUInt(uint n, Benchmark::Record *benchmark) {
        this->socket_.Write((uchar *)&n, sizeof(uint), benchmark);
    }

    uint ReadUInt(Benchmark::Record *benchmark) {
        uint n;
        this->socket_.Read((uchar *)&n, sizeof(uint), benchmark);
        return n;
    }

    void WriteUInt64(uint64_t n, Benchmark::Record *benchmark) {
        this->socket_.Write((uchar *)&n, sizeof(uint64_t), benchmark);
    }

    uint64_t ReadUInt64(Benchmark::Record *benchmark) {
        uint64_t n;
        this->socket_.Read((uchar *)&n, sizeof(uint64_t), benchmark);
        return n;
    }

    template <typename D>
    void ReadData(D &data, Benchmark::Record *benchmark) {
        // fprintf(stderr, "ReadData, size = %u\n", size);
        uint data_size = data.Size();
        std::vector<uchar> buffer(data_size);
        this->socket_.Read(buffer.data(), buffer.size(), benchmark);
        data.LoadBuffer(buffer.data());
    }

    template <typename D>
    void ReadDataVector(std::vector<D> &data, Benchmark::Record *benchmark) {
        uint data_size = data[0].Size();
        uint buffer_size = data.size() * data_size;
        std::vector<uchar> buffer(buffer_size);
        this->socket_.Read(buffer.data(), buffer.size(), benchmark);
        for (uint i = 0; i < data.size(); i++) {
            data[i].LoadBuffer(buffer.data() + i * data_size);
        }
    }

    template <typename D>
    void WriteDataVector(std::vector<D> &data, Benchmark::Record *benchmark) {
        uint data_size = data[0].Size();
        std::vector<uchar> buffer(data.size() * data_size);
        for (uint i = 0; i < data.size(); i++) {
            data[i].DumpBuffer(buffer.data() + i * data_size);
        }
        this->socket_.Write(buffer.data(), buffer.size(), benchmark);
    }

    template <typename D>
    void WriteData(D &data, Benchmark::Record *benchmark) {
        // fprintf(stderr, "WriteData, size = %u\n", data.Size());
        const std::vector<uchar> dump = data.DumpVector();
        this->socket_.Write(dump.data(), dump.size(), benchmark);
    }

    void Close() {
        this->socket_.Close();
    }
};

#endif /* PEER_H_ */
