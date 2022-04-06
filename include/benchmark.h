#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <inttypes.h>

#include <chrono>

#include "util.h"

namespace Benchmark {

using namespace std::chrono;

typedef high_resolution_clock::time_point timestamp;

class Record {
private:
    timestamp t1_;
    duration<long long, std::nano> duration_;

public:
    uint64_t count_;
    uint64_t bandwidth_;

    Record();

    inline void Start() {
        this->t1_ = high_resolution_clock::now();
    }

    inline void End() {
        this->duration_ += high_resolution_clock::now() - this->t1_;
        this->count_++;
    }

    uint64_t Time() {
        return duration_cast<microseconds>(this->duration_).count();
    }
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
#define BENCHMARK_DPF_GEN
extern Record DPF_GEN;
#define BENCHMARK_DPF_EVAL
extern Record DPF_EVAL;
#define BENCHMARK_DPF_EVAL_FULL
extern Record DPF_EVAL_ALL;

// PSEUDO_DPF
#define BENCHMARK_PSEUDO_DPF_GEN
extern Record PSEUDO_DPF_GEN;
#define BENCHMARK_PSEUDO_DPF_EVAL
extern Record PSEUDO_DPF_EVAL;
#define BENCHMARK_PSEUDO_DPF_EVAL_FULL
extern Record PSEUDO_DPF_EVAL_ALL;

};  // namespace Benchmark

#endif /* BENCHMARK_H_ */
