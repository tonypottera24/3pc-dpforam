#ifndef PEER_H_
#define PEER_H_

#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>

#include "socket.h"
#include "typedef.h"

class Peer {
private:
    Socket socket_;
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption prg_;

public:
    Peer() {}
    Peer(Socket socket, CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption prg);

    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption &PRG();
    Socket &Socket();
    uint Bandwidth();

    void WriteUInt(uint n, bool count_band);
    uint ReadUInt();

    void WriteLong(uint64_t n, bool count_band);
    uint64_t ReadLong();

    template <typename D>
    D ReadData(const uint size) {
        // fprintf(stderr, "ReadData, size = %u\n", size);
        uchar binary_data[size];
        this->socket_.Read(binary_data, size);
        return D(binary_data, size);
    }

    template <typename D>
    void WriteData(D data, bool count_band) {
        // fprintf(stderr, "WriteData, size = %u\n", data.Size());
        uchar buffer[data.Size()];
        data.Dump(buffer);
        this->socket_.Write(buffer, data.Size(), count_band);
    }

    template <typename D>
    std::vector<D> ReadData(const uint size, const uint data_size) {
        uint buffer_size = size * data_size;
        uchar buffer[buffer_size];
        this->socket_.Read(buffer, buffer_size);
        std::vector<D> data;
        for (uint i = 0; i < size; i++) {
            data.emplace_back(&buffer[i * data_size], data_size);
        }
        return data;
    }

    template <typename D>
    void WriteData(std::vector<D> &data, bool count_band) {
        uint data_size = data[0].Size();
        uint buffer_size = data.size() * data_size;
        uchar buffer[buffer_size];
        for (uint i = 0; i < data.size(); i++) {
            uchar data_buffer[data_size];
            data[i].Dump(data_buffer);
            memcpy(&buffer[i * data_size], data_buffer, data_size);
        }
        this->socket_.Write(buffer, buffer_size, count_band);
    }

    void Close();
};

#endif /* PEER_H_ */
