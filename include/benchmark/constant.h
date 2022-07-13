#ifndef BENCHMARK_CONSTANT_H_
#define BENCHMARK_CONSTANT_H_

#include "benchmark/data_record.h"
#include "benchmark/record.h"

namespace Benchmark {

// Top Level ORAM
extern Record KEY_TO_INDEX;
extern Record ORAM_READ;
extern Record ORAM_WRITE;
extern Record ORAM_POSITION_MAP;

// KEY_VALUE
#define BENCHMARK_KEY_VALUE
extern Record KEY_VALUE_PREPARE;
extern Record KEY_VALUE_DPF_GEN;
extern Record KEY_VALUE_DPF_EVAL;
extern Record KEY_VALUE_ADD_INDEX;
#define BENCHMARK_KEY_VALUE_HASH
extern std::vector<Record> KEY_VALUE_HASH;

// #define BENCHMARK_PIR
extern Record PIR_GEN_DPF;
extern Record PIR_EVAL_DPF;
extern Record PIR_ADD_DATA;
extern Record PIR_GROUP_PREPARE;

// #define BENCHMARK_PIW
extern Record PIW_GEN_DPF;
extern Record PIW_EVAL_DPF;
extern Record PIW_ADD_DATA;
extern Record PIW_GROUP_PREPARE;

// #define BENCHMARK_DPF
extern Record DPF_GEN;
extern Record DPF_EVAL;
extern Record DPF_EVAL_ALL;

// #define BENCHMARK_PSEUDO_DPF
extern Record PSEUDO_DPF_GEN;
extern Record PSEUDO_DPF_EVAL;
extern Record PSEUDO_DPF_EVAL_ALL;

// #define BENCHMARK_SSOT
extern Record SSOT;

// #define BENCHMARK_SOCKET
extern Record SOCKET_WRITE;
extern Record SOCKET_READ;

// #define BENCHMARK_BINARY_DATA
extern DataRecord BINARY_DATA;

// #define BENCHMARK_ZP_DATA
extern DataRecord ZP_DATA;

};  // namespace Benchmark

#endif /* BENCHMARK_H_ */
