#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "typedef.h"

class Connection {
public:
    unsigned long bandwidth_ = 0u;

    virtual ~Connection();
    virtual void InitServer(int port) = 0;
    virtual void InitClient(const char *ip, int port) = 0;
    virtual void SetNoDelay() = 0;
    virtual void Write(const uchar *data, unsigned long bytes,
                       bool count_band = true) = 0;
    virtual void Read(uchar *data, unsigned long bytes) = 0;
    // virtual void fwrite(const uchar *data, unsigned long bytes,
    //                     bool count_band = true) = 0;
    // virtual void fread(uchar *data, unsigned long bytes) = 0;
    virtual void Flush() = 0;
    virtual void Close() = 0;
    void WriteInt(int n, bool count_band = true);
    int ReadInt();
    void WriteLong(long n, bool count_band = true);
    long ReadLong();
};

#endif /* CONNECTION_H_ */
