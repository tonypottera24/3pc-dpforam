#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <cstdint>

#include "data.h"
#include "typedef.h"

class Connection {
public:
    uint64_t bandwidth_ = 0;
    virtual ~Connection();
    virtual void InitServer(const char *ip, const uint port) = 0;
    virtual void InitClient(const char *ip, uint port) = 0;
    virtual void SetNoDelay() = 0;
    virtual void Write(const uchar *data, uint64_t bytes,
                       bool count_band = true) = 0;
    virtual void Read(uchar *data, uint64_t bytes) = 0;
    // virtual void fwrite(const uchar *data, uint64_t bytes,
    //                     bool count_band = true) = 0;
    // virtual void fread(uchar *data, uint64_t bytes) = 0;
    virtual void Flush() = 0;
    virtual void Close() = 0;

    void WriteLong(uint64_t n, bool count_band = true);
    uint64_t ReadLong();

    template <typename D>
    void WriteData(D &data, bool count_band = true) {
        // fprintf(stderr, "WriteData, size = %u\n", data.Size());
        Write(data.Dump(), data.Size(), count_band);
    }

    template <typename D>
    D &ReadData(const DataType data_type, const uint size) {
        // fprintf(stderr, "ReadData, size = %u\n", size);
        uchar binary_data[size];
        Read(binary_data, size);
        return *new D(data_type, binary_data, size);
    }

    template <typename D>
    void WriteData(std::vector<D> &data, bool count_band = true) {
        uint data_size = data[0].Size();
        uint buffer_size = data.size() * data_size;
        uchar buffer[buffer_size];
        for (uint i = 0; i < data.size(); i++) {
            memcpy(&buffer[i * data_size], data[i].Dump(), data_size);
        }
        Write(buffer, buffer_size, count_band);
    }

    template <typename D>
    std::vector<D> &ReadData(const DataType data_type, const uint size, const uint data_size) {
        uint buffer_size = size * data_size;
        uchar buffer[buffer_size];
        Read(buffer, buffer_size);
        std::vector<D> *data = new std::vector<D>;
        for (uint i = 0; i < size; i++) {
            data->emplace_back(data_type, &buffer[i * data_size], data_size);
        }
        return *data;
    }
};

#endif /* CONNECTION_H_ */
