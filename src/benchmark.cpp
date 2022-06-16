#include "benchmark.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

Benchmark::Record::Record(std::string name) {
    this->name = name;
    this->duration_ = duration<long long, std::nano>::zero();
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
    this->count_ = this->count_ < rhs.count_ ? 0 : this->count_ - rhs.count_;
    this->bandwidth_ -= rhs.bandwidth_;
    return *this;
}

uint64_t Benchmark::Record::GetTime() {
    return duration_cast<microseconds>(this->duration_).count();
}

void Benchmark::Record::Print() {
    json j = {
        {"name", this->name},
        {"time", this->GetTime()},
        {"ct", this->count_},
        {"bandwidth", this->bandwidth_},
    };
    std::cout << j.dump() << std::endl;
    // fprintf(stderr, "time: %lu\n", this->GetTime());
    // fprintf(stderr, "count: %lu\n", this->count_);
    // fprintf(stderr, "bandwidth: %lu\n", this->bandwidth_);
}

void Benchmark::Record::PrintTotal(Peer peer[2], uint64_t iteration) {
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

    uint64_t avg_bandwidth = this->bandwidth_;
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->bandwidth_, NULL);
        avg_bandwidth += peer[1 - b].ReadUInt64();
    }
    avg_bandwidth /= 3.0;

    json j = {
        {"name", this->name},
        {"time", avg_time / iteration},
        {"ct", avg_count / iteration},
        {"bandwidth", avg_bandwidth / iteration},
    };
    std::cerr << j.dump() << std::endl;

    // fprintf(stderr, "%s %lu %lu %lu\n", title, avg_time / iteration, avg_count / iteration, avg_bandwidth / iteration);
}

// Top Level ORAM
Benchmark::Record Benchmark::KEY_TO_INDEX("KEY_TO_INDEX");
Benchmark::Record Benchmark::ORAM_READ("ORAM_READ");
Benchmark::Record Benchmark::ORAM_WRITE("ORAM_WRITE");

// Position map ORAM
Benchmark::Record Benchmark::ORAM_POSITION_MAP("ORAM_POSITION_MAP");

// BENCHMARK_KEY_VALUE
Benchmark::Record Benchmark::KEY_VALUE_PREPARE("KEY_VALUE_PREPARE");
Benchmark::Record Benchmark::KEY_VALUE_DPF("KEY_VALUE_DPF");
Benchmark::Record Benchmark::KEY_VALUE_EVALUATE("KEY_VALUE_EVALUATE");
// BENCHMARK_KEY_VALUE_HASH
Benchmark::Record Benchmark::KEY_VALUE_HASH[2] = {Benchmark::Record("KEY_VALUE_HASH[0]"), Benchmark::Record("KEY_VALUE_HASH[1]")};

// BENCHMARK_GROUP_PREPARE
Benchmark::Record Benchmark::GROUP_PREPARE_READ("GROUP_PREPARE_READ");
Benchmark::Record Benchmark::GROUP_PREPARE_WRITE("GROUP_PREPARE_WRITE");

// BENCHMARK_DPF
Benchmark::Record Benchmark::DPF_GEN("DPF_GEN");
Benchmark::Record Benchmark::DPF_EVAL("DPF_EVAL");
Benchmark::Record Benchmark::DPF_EVAL_ALL("DPF_EVAL_ALL");

// BENCHMARK_PSEUDO_DPF
Benchmark::Record Benchmark::PSEUDO_DPF_GEN("PSEUDO_DPF_GEN");
Benchmark::Record Benchmark::PSEUDO_DPF_EVAL("PSEUDO_DPF_EVAL");
Benchmark::Record Benchmark::PSEUDO_DPF_EVAL_ALL("PSEUDO_DPF_EVAL_ALL");

// BENCHMARK_BINARY_DATA
Benchmark::Record Benchmark::BINARY_DATA_COPY("BINARY_DATA_COPY");
Benchmark::Record Benchmark::BINARY_DATA_ARITHMATIC("BINARY_DATA_ARITHMATIC");
Benchmark::Record Benchmark::BINARY_DATA_DUMP_LOAD("BINARY_DATA_DUMP_LOAD");
Benchmark::Record Benchmark::BINARY_DATA_RANDOM("BINARY_DATA_RANDOM");

Benchmark::Record Benchmark::BINARY_DATA_COPY_CACHE("BINARY_DATA_COPY_CACHE");
Benchmark::Record Benchmark::BINARY_DATA_ARITHMATIC_CACHE("BINARY_DATA_ARITHMATIC_CACHE");
Benchmark::Record Benchmark::BINARY_DATA_DUMP_LOAD_CACHE("BINARY_DATA_DUMP_LOAD_CACHE");
Benchmark::Record Benchmark::BINARY_DATA_RANDOM_CACHE("BINARY_DATA_RANDOM_CACHE");