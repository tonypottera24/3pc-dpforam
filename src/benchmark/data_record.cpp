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
    this->copy_.PrintTotal(peer, iterations);
    this->compare_.PrintTotal(peer, iterations);
    this->arithmatic_.PrintTotal(peer, iterations);
    this->dump_load_.PrintTotal(peer, iterations);
    this->random_.PrintTotal(peer, iterations);

    Record others = total - this->copy_ - this->compare_ - this->arithmatic_ - this->dump_load_ - this->random_;
    others.name = this->name_ + "_OTHERS";
    others.PrintTotal(peer, iterations);
}
