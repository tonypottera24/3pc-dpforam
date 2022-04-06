#include "benchmark.h"

Benchmark::Record::Record() {
    this->duration_ = duration<long long, std::nano>::zero();
    this->count_ = 0;
    this->bandwidth_ = 0;
}

Benchmark::Record &Benchmark::Record::operator=(const Benchmark::Record &other) {
    // copy operation
    if (this == &other) return *this;
    this->duration_ = other.duration_;
    this->count_ = other.count_;
    this->bandwidth_ = other.bandwidth_;
    return *this;
}

Benchmark::Record &Benchmark::Record::operator+=(const Benchmark::Record &rhs) {
    this->duration_ += rhs.duration_;
    this->count_ += rhs.count_;
    this->bandwidth_ += rhs.bandwidth_;
    return *this;
}

Benchmark::Record &Benchmark::Record::operator-=(const Benchmark::Record &rhs) {
    this->duration_ -= rhs.duration_;
    this->count_ -= rhs.count_;
    this->bandwidth_ -= rhs.bandwidth_;
    return *this;
}

uint64_t Benchmark::Record::GetTime() {
    return duration_cast<microseconds>(this->duration_).count();
}

void Benchmark::Record::AddBandwidth(uint64_t n) {
    this->bandwidth_ += n;
}

void Benchmark::Record::Print() {
    fprintf(stderr, "time: %llu\n", this->GetTime());
    fprintf(stderr, "count: %llu\n", this->count_);
    fprintf(stderr, "bandwidth: %llu\n", this->bandwidth_);
}

void Benchmark::Record::PrintTotal(Peer peer[2], const char *title, uint64_t iteration) {
    uint64_t avg_time = this->GetTime();
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->GetTime(), NULL);
        avg_time += peer[1 - b].ReadUInt64();
    }
    avg_time /= 3.0;

    uint64_t avg_count = this->count_;
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->count_, NULL);
        avg_count += peer[1 - b].ReadUInt64();
    }
    avg_count /= 3.0;

    uint64_t total_bandwidth = this->bandwidth_;
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->bandwidth_, NULL);
        total_bandwidth += peer[1 - b].ReadUInt64();
    }

    fprintf(stderr, "%s: (%llu, %llu, %llu)\n", title, avg_time / iteration, avg_count / iteration, total_bandwidth / iteration);
}

// Top Level ORAM
Benchmark::Record Benchmark::ORAM_READ;
Benchmark::Record Benchmark::ORAM_WRITE;
Benchmark::Record Benchmark::KEY_TO_INDEX;

// Position map ORAM
Benchmark::Record Benchmark::ORAM_READ_POSITION_MAP;
Benchmark::Record Benchmark::ORAM_WRITE_POSITION_MAP;

// DPF
Benchmark::Record Benchmark::DPF_GEN;
Benchmark::Record Benchmark::DPF_EVAL;
Benchmark::Record Benchmark::DPF_EVAL_ALL;

// PSEUDO_DPF
Benchmark::Record Benchmark::PSEUDO_DPF_GEN;
Benchmark::Record Benchmark::PSEUDO_DPF_EVAL;
Benchmark::Record Benchmark::PSEUDO_DPF_EVAL_ALL;
