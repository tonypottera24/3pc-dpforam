#ifndef BENCHMARK_DATA_RECORD_H_
#define BENCHMARK_DATA_RECORD_H_

#include "benchmark/record.h"

namespace Benchmark {

class DataRecord {
private:
    std::string name_;
    Record copy_cache_;
    Record compare_cache_;
    Record arithmatic_cache_;
    Record dump_load_cache_;
    Record random_cache_;

public:
    Record copy_;
    Record compare_;
    Record arithmatic_;
    Record dump_load_;
    Record random_;

public:
    DataRecord(std::string name = std::string());

    void Start() {
        copy_cache_ = copy_;
        compare_cache_ = compare_;
        arithmatic_cache_ = arithmatic_;
        dump_load_cache_ = dump_load_;
        random_cache_ = random_;
    }

    void Stop() {
        copy_ = copy_cache_;
        compare_ = compare_cache_;
        arithmatic_ = arithmatic_cache_;
        dump_load_ = dump_load_cache_;
        random_ = random_cache_;
    }

    void PrintTotal(Peer peer[2], uint iterations, Record total);
};
};  // namespace Benchmark

#endif