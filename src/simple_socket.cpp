#include "simple_socket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#define BUFF_BYTES 1024 * 16

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void SimpleSocket::SetStream() {
    stream_ = fdopen(socket_fd_, "wb+");
    buffer_ = new char[BUFF_BYTES];
    memset(buffer_, 0, BUFF_BYTES);
    setvbuf(stream_, buffer_, _IOFBF, BUFF_BYTES);
}

void SimpleSocket::InitServer(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        error("InitServer: socket failed");
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        error("InitServer: setsockopt failed");
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        error("InitServer: bind failed");
    }
    if (listen(server_fd, 1) < 0) {
        error("InitServer: listen failed");
    }
    int addr_len = sizeof(address);
    socket_fd_ = accept(server_fd, (struct sockaddr *)&address,
                        (socklen_t *)&addr_len);
    if (socket_fd_ < 0) {
        error("InitServer: accept failed");
    }
    ::close(server_fd);
    SetStream();
    SetNoDelay();
}

void SimpleSocket::InitClient(const char *ip, int port) {
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        error("InitClient: socket failed");
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        error("InitClient: inet_pton failed");
    }
    if (connect(socket_fd_, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        error("InitClient: connect failed");
    }
    SetStream();
    SetNoDelay();
}

void SimpleSocket::SetNoDelay() {
    int opt = 1;
    if (setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        error("SetNoDelay: setsockopt failed");
    }
}

void SimpleSocket::Write(const uchar *data, unsigned long bytes, bool count_band) {
    long write_bytes;
    unsigned long offset = 0ul;
    while (offset < bytes) {
        write_bytes = ::write(socket_fd_, data + offset, bytes - offset);
        if (write_bytes < 0) {
            error("write failed");
        }
        offset += write_bytes;
    }
    if (count_band) {
        bandwidth += bytes;
    }
}

void SimpleSocket::Read(uchar *data, unsigned long bytes) {
    long read_bytes;
    unsigned long offset = 0ul;
    while (offset < bytes) {
        read_bytes = ::read(socket_fd_, data + offset, bytes - offset);
        if (read_bytes < 0) {
            error("read failed");
        }
        offset += read_bytes;
    }
}

// TODO: debug
// void SimpleSocket::fwrite(const uchar *data, unsigned long bytes, bool count_band)
// {
//     long write_bytes;
//     unsigned long offset = 0ul;
//     while (offset < bytes)
//     {
//         write_bytes = ::fwrite(data + offset, 1, bytes - offset, stream);
//         if (write_bytes < 0)
//         {
//             error("write failed");
//         }
//         offset += write_bytes;
//     }
//     if (count_band)
//     {
//         bandwidth += bytes;
//     }
// }

// TODO: debug
// void SimpleSocket::fread(uchar *data, unsigned long bytes)
// {
//     long read_bytes;
//     unsigned long offset = 0ul;
//     while (offset < bytes)
//     {
//         read_bytes = ::fread(data + offset, 1, bytes - offset, stream);
//         if (read_bytes < 0)
//         {
//             error("read failed");
//         }
//         offset += read_bytes;
//     }
// }

void SimpleSocket::Flush() {
    fflush(stream_);
}

void SimpleSocket::Close() {
    fclose(stream_);
    delete[] buffer_;
    ::close(socket_fd_);
}
