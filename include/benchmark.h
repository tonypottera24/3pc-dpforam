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
    uint64_t bandwidth_;
    uint64_t count_;

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
    }

    inline void End() {
        this->duration_ += high_resolution_clock::now() - this->last_start_time_;
        this->count_++;
    }

    void AddBandwidth(uint64_t n);
    uint64_t GetTime();

    void Print();
    void PrintTotal(Peer peer[2], const char *title = "", uint64_t iteration = 1);
};

// Top Level ORAM
extern Record ORAM_READ;
extern Record ORAM_WRITE;
extern Record KEY_TO_INDEX;

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

// DPF
// #define BENCHMARK_DPF_GEN
extern Record DPF_GEN;
// #define BENCHMARK_DPF_EVAL
extern Record DPF_EVAL;
// #define BENCHMARK_DPF_EVAL_FULL
extern Record DPF_EVAL_ALL;

// PSEUDO_DPF
// #define BENCHMARK_PSEUDO_DPF_GEN
extern Record PSEUDO_DPF_GEN;
// #define BENCHMARK_PSEUDO_DPF_EVAL
extern Record PSEUDO_DPF_EVAL;
// #define BENCHMARK_PSEUDO_DPF_EVAL_FULL
extern Record PSEUDO_DPF_EVAL_ALL;

};  // namespace Benchmark

#endif /* BENCHMARK_H_ */
