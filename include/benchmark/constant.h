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

// PIR
// extern Record DPF_PIR;
// extern Record DPF_KEY_PIR;
// extern Record SSOT_PIR;

// PIW
// extern Record DPF_PIW;

// KEY_VALUE
// #define BENCHMARK_KEY_VALUE
extern Record KEY_VALUE_PREPARE;
extern Record KEY_VALUE_DPF;
extern Record KEY_VALUE_EVALUATE;
// #define BENCHMARK_KEY_VALUE_HASH
extern Record KEY_VALUE_HASH[2];

// #define BENCHMARK_GROUP_PREPARE
extern Record GROUP_PREPARE_READ;
extern Record GROUP_PREPARE_WRITE;

// #define BENCHMARK_DPF
extern Record DPF_GEN;
extern Record DPF_EVAL;
extern Record DPF_EVAL_ALL;

// #define BENCHMARK_PSEUDO_DPF
extern Record PSEUDO_DPF_GEN;
extern Record PSEUDO_DPF_EVAL;
extern Record PSEUDO_DPF_EVAL_ALL;

#define BENCHMARK_SOCKET
extern Record SOCKET_WRITE;
extern Record SOCKET_READ;

// #define BENCHMARK_BINARY_DATA
extern DataRecord BINARY_DATA;

// #define BENCHMARK_ZP_DATA
extern DataRecord ZP_DATA;

};  // namespace Benchmark

#endif /* BENCHMARK_H_ */
