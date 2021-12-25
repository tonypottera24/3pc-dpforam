#ifndef SIMPLE_SOCKET_H_
#define SIMPLE_SOCKET_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <vector>

#include "typedef.h"

class Socket {
private:
    int socket_fd_;
    FILE *stream_;
    char *buffer_;
    uint bandwidth_ = 0;

    void SetStream();

public:
    void InitServer(const char *server_ip, const uint port);
    void InitClient(const char *ip, uint port);
    void SetNoDelay();
    void Write(const uchar *data, uint data_size, bool count_band = true);
    void Read(uchar *data, uint data_size);
    void Flush();
    void Close();
    uint Bandwidth();
};

#endif /* SIMPLE_SOCKET_H_ */
