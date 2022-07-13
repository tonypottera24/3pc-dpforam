#include "benchmark/data_record.h"

Benchmark::DataRecord::DataRecord(std::string name) {
    this->name_ = name;
    this->copy_.name = name + "_COPY";
    this->compare_.name = name + "_COMPARE";
    this->arithmatic_.name = name + "_ARITHMATIC";
    this->dump_load_.name = name + "_DUMP_LOAD";
    this->random_.name = name + "_RANDOM";
}

void Benchmark::DataRecord::PrintTotal(Peer peer[2], uint iterations, Record total) {
    copy_ = copy_cache_;
    compare_ = compare_cache_;
    arithmatic_ = arithmatic_cache_;
    dump_load_ = dump_load_cache_;
    random_ = random_cache_;

    this->copy_.Sync(peer);
    this->copy_.PrintTotal(iterations);

    this->compare_.Sync(peer);
    this->compare_.PrintTotal(iterations);

    this->arithmatic_.Sync(peer);
    this->arithmatic_.PrintTotal(iterations);

    this->dump_load_.Sync(peer);
    this->dump_load_.PrintTotal(iterations);

    this->random_.Sync(peer);
    this->random_.PrintTotal(iterations);

    Record others = total - this->copy_ - this->compare_ - this->arithmatic_ - this->dump_load_ - this->random_;
    others.name = this->name_ + "_OTHERS";
    others.PrintTotal(iterations);
}
