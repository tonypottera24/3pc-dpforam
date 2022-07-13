#include "benchmark/record.h"

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
    this->bandwidth_ = this->bandwidth_ < rhs.bandwidth_ ? 0 : this->bandwidth_ - rhs.bandwidth_;
    return *this;
}

uint64_t Benchmark::Record::GetTime() {
    return duration_cast<microseconds>(this->duration_).count();
}

void Benchmark::Record::Sync(Peer peer[2]) {
    uint64_t time[3], count[3], bandwidth[3];
    time[2] = this->GetTime();
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->GetTime(), NULL);
        time[b] = peer[1 - b].ReadUInt64(NULL);
    }
    std::sort(time, time + 3);
    this->duration_ = duration<long long, std::micro>(time[2]);

    count[2] = this->count_;
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->count_, NULL);
        count[b] = peer[1 - b].ReadUInt64(NULL);
    }
    std::sort(count, count + 3);
    this->count_ = count[2];

    bandwidth[2] = this->bandwidth_;
    for (uint b = 0; b < 2; b++) {
        peer[b].WriteUInt64(this->bandwidth_, NULL);
        bandwidth[b] = peer[1 - b].ReadUInt64(NULL);
    }
    std::sort(bandwidth, bandwidth + 3);
    this->bandwidth_ = bandwidth[2];
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

void Benchmark::Record::PrintTotal(uint64_t iteration) {
    json j = {
        {"name", this->name},
        {"time", this->GetTime() / iteration},
        {"ct", this->count_ / iteration},
        {"bandwidth", this->bandwidth_ / iteration},
    };
    std::cerr << j.dump() << std::endl;

    // fprintf(stderr, "%s %lu %lu %lu\n", title, avg_time / iteration, avg_count / iteration, avg_bandwidth / iteration);
}