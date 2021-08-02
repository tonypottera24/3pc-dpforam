#ifndef SIMPLE_SOCKET_H_
#define SIMPLE_SOCKET_H_

#include <stdio.h>

#include "connection.h"

class SimpleSocket : public Connection {
private:
    int socket_fd_;
    FILE *stream_;
    char *buffer_;

    void SetStream();

public:
    void InitServer(int port);
    void InitClient(const char *ip, int port);
    void SetNoDelay();
    void Write(const uchar *data, uint64_t bytes, bool count_band = true);
    void Read(uchar *data, uint64_t bytes);
    // void fwrite(const uchar *data, uint64_t bytes, bool count_band = true);
    // void fread(uchar *data, uint64_t bytes);
    void Flush();
    void Close();
};

#endif /* SIMPLE_SOCKET_H_ */
