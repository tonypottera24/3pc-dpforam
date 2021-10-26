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
    void WriteData(Data &data, bool count_band = true);
    Data &ReadData(const DataType data_type, const uint size);
    void WriteData(std::vector<Data> &data, bool count_band = true);
    std::vector<Data> &ReadData(const DataType data_type, const uint size, const uint data_size);
};

#endif /* CONNECTION_H_ */
