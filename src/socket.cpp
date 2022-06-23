#include "socket.h"

#include "benchmark/constant.h"

#define BUFF_BYTES 1024 * 16

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void Socket::SetStream() {
    stream_ = fdopen(socket_fd_, "wb+");
    // buffer_ = new char[BUFF_BYTES];
    // memset(buffer_, 0, BUFF_BYTES);
    // setvbuf(stream_, buffer_, _IOFBF, BUFF_BYTES);
}

void Socket::InitServer(const char *ip, const uint port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        error("InitServer: socket failed");
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        error("InitServer: setsockopt failed");
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
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

void Socket::InitClient(const char *ip, uint port) {
    int connect_status = -1;
    while (connect_status < 0) {
        socket_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_fd_ < 0) {
            error("InitClient: socket failed");
        }
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
            error("InitClient: inet_pton failed");
        }
        connect_status = connect(socket_fd_, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (connect_status < 0) {
            close(socket_fd_);
            fprintf(stderr, "InitClient: connect failed, wait for 3 sec...\n");
            sleep(3);
        }
    }
    SetStream();
    SetNoDelay();
}

void Socket::SetNoDelay() {
    int opt = 1;
    if (setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
        error("SetNoDelay: setsockopt failed");
    }
}

void Socket::Write(const uchar *data, uint data_size, Benchmark::Record *benchmark) {
    // fprintf(stderr, "Write, data_size = %u\n", data_size);
#ifdef BENCHMARK_SOCKET
    if (benchmark != NULL) {
        Benchmark::SOCKET_WRITE.Start();
    }
#endif
    uint offset = 0;
    while (offset < data_size) {
        // fprintf(stderr, "Write, offset = %u\n", offset);
        int write_size = ::write(socket_fd_, data + offset, data_size - offset);
        if (write_size < 0) {
            error("write failed");
        }
        offset += write_size;
    }
    if (benchmark != NULL) {
        benchmark->bandwidth_ += data_size;
    }
    // fflush(stream_);
#ifdef BENCHMARK_SOCKET
    if (benchmark != NULL) {
        Benchmark::SOCKET_WRITE.Stop(data_size);
    }
#endif
}

void Socket::Read(uchar *data, uint data_size, Benchmark::Record *benchmark) {
#ifdef BENCHMARK_SOCKET
    if (benchmark != NULL) {
        Benchmark::SOCKET_READ.Start();
    }
#endif
    uint offset = 0;
    while (offset < data_size) {
        int read_size = ::read(socket_fd_, data + offset, data_size - offset);
        if (read_size < 0) {
            error("read failed");
        }
        offset += read_size;
    }
#ifdef BENCHMARK_SOCKET
    if (benchmark != NULL) {
        Benchmark::SOCKET_READ.Stop();
    }
#endif
}

void Socket::Flush() {
    fflush(stream_);
}

void Socket::Close() {
    fclose(stream_);
    // delete[] buffer_;
    ::close(socket_fd_);
}