#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "typedef.h"

class Connection {
public:
    unsigned long bandwidth = 0u;

    virtual ~Connection();
    virtual void InitServer(int port) = 0;
    virtual void InitClient(const char *ip, int port) = 0;
    virtual void set_no_delay() = 0;
    virtual void Write(const uchar *data, unsigned long bytes,
                       bool count_band = true) = 0;
    virtual void read(uchar *data, unsigned long bytes) = 0;
    // virtual void fwrite(const uchar *data, unsigned long bytes,
    //                     bool count_band = true) = 0;
    // virtual void fread(uchar *data, unsigned long bytes) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    void WriteInt(int n, bool count_band = true);
    int ReadInt();
    void WriteLong(long n, bool count_band = true);
    long ReadLong();
};

#endif /* CONNECTION_H_ */
