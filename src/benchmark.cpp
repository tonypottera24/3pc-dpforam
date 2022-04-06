#include "benchmark.h"

Benchmark::Record::Record() {
    this->duration_ = duration<long long, std::nano>::zero();
    this->count_ = 0;
    this->bandwidth_ = 0;
}

// Top Level ORAM
Benchmark::Record ORAM_READ;
Benchmark::Record ORAM_WRITE;
Benchmark::Record KEY_TO_INDEX;

// Position map ORAM
Benchmark::Record ORAM_READ_POSITION_MAP;
Benchmark::Record ORAM_WRITE_POSITION_MAP;

// DPF
Benchmark::Record DPF_GEN;
Benchmark::Record DPF_EVAL;
Benchmark::Record DPF_EVAL_ALL;

// PSEUDO_DPF
Benchmark::Record PSEUDO_DPF_GEN;
Benchmark::Record PSEUDO_DPF_EVAL;
Benchmark::Record PSEUDO_DPF_EVAL_ALL;
