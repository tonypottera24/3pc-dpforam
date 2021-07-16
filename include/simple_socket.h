#ifndef SIMPLE_SOCKET_H_
#define SIMPLE_SOCKET_H_

#include <stdio.h>

#include "connection.h"

class SimpleSocket : public Connection {
private:
    int socket_fd;
    FILE *stream;
    char *buffer;

    void set_stream();

public:
    void InitServer(int port);
    void InitClient(const char *ip, int port);
    void SetNoDelay();
    void Write(const uchar *data, unsigned long bytes, bool count_band = true);
    void Read(uchar *data, unsigned long bytes);
    // void fwrite(const uchar *data, unsigned long bytes, bool count_band = true);
    // void fread(uchar *data, unsigned long bytes);
    void Flush();
    void Close();
};

#endif /* SIMPLE_SOCKET_H_ */
