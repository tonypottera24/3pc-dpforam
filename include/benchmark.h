#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <inttypes.h>
#include <sys/types.h>

#include <chrono>
#include <cstdio>

#include "peer.h"

namespace Benchmark {

using namespace std::chrono;

typedef high_resolution_clock::time_point timestamp;

class Record {
private:
    timestamp last_start_time_;
    duration<long long, std::nano> duration_;
    uint64_t last_bandwidth_ = 0;
    uint64_t bandwidth_ = 0;
    uint64_t count_ = 0;

public:
    Record();

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

    inline void Start() {
        this->last_start_time_ = high_resolution_clock::now();
        this->last_bandwidth_ = this->bandwidth_;
    }

    inline uint64_t End() {
        this->duration_ += high_resolution_clock::now() - this->last_start_time_;
        this->count_++;
        return this->bandwidth_ - this->last_bandwidth_;
    }

    void AddBandwidth(uint64_t n);
    uint64_t GetTime();

    void Print();
    void PrintTotal(Peer peer[2], const char *title = "", uint64_t iteration = 1);
};

// Top Level ORAM
extern Record KEY_TO_INDEX;
extern uint KEY_TO_INDEX_COLLISION;
extern Record ORAM_READ;
extern Record ORAM_WRITE;

// Position map ORAM
#define BENCHMARK_POSITION_MAP
extern Record ORAM_READ_POSITION_MAP;
extern Record ORAM_WRITE_POSITION_MAP;

// PIR
// extern Record DPF_PIR;
// extern Record DPF_KEY_PIR;
// extern Record SSOT_PIR;

// PIW
// extern Record DPF_PIW;

// KEY_VALUE
#define BENCHMARK_KEY_VALUE
extern Record KEY_VALUE_PREPARE;
extern Record KEY_VALUE_DPF;
extern Record KEY_VALUE_EVALUATE;
// #define BENCHMARK_KEY_VALUE_HASH
extern Record KEY_VALUE_HASH[2];

// #define BENCHMARK_GROUP_PREPARE
extern Record GROUP_PREPARE_READ;
extern Record GROUP_PREPARE_WRITE;

// #define BENCHMARK_DPF
extern Record DPF_GEN;
extern Record DPF_EVAL;
extern Record DPF_EVAL_ALL;

// #define BENCHMARK_PSEUDO_DPF
extern Record PSEUDO_DPF_GEN;
extern Record PSEUDO_DPF_EVAL;
extern Record PSEUDO_DPF_EVAL_ALL;

};  // namespace Benchmark

#endif /* BENCHMARK_H_ */
