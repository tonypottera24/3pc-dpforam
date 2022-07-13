#ifndef BENCHMARK_RECORD_H_
#define BENCHMARK_RECORD_H_

#include <inttypes.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "peer.h"

namespace Benchmark {

using namespace std::chrono;
using json = nlohmann::json;

typedef high_resolution_clock::time_point timestamp;

class Record {
private:
    timestamp last_start_time_;

public:
    std::string name;
    duration<long long, std::nano> duration_ = duration<long long, std::nano>::zero();
    uint64_t bandwidth_ = 0;
    uint64_t count_ = 0;

public:
    Record(std::string name = std::string());

    Record &operator=(const Record &other);
    Record &operator+=(const Record &rhs);
    Record &operator-=(const Record &rhs);

    friend Record operator+(Record lhs, const Record &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend Record operator-(Record lhs, const Record &rhs) {
        lhs -= rhs;
        return lhs;
    }

    void Start() {
        this->last_start_time_ = high_resolution_clock::now();
    }

    void Stop(uint64_t extra_bandwidth = 0) {
        this->duration_ += high_resolution_clock::now() - this->last_start_time_;
        this->count_++;
        this->bandwidth_ += extra_bandwidth;
    }

    uint64_t GetTime();

    void Sync(Peer peer[2]);

    void Print();
    void PrintTotal(uint64_t iteration = 1);
};

};  // namespace Benchmark

#endif