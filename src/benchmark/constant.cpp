#include "benchmark/constant.h"

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
Benchmark::DataRecord Benchmark::BINARY_DATA("BINARY_DATA");
// BENCHMARK_ZP_DATA
Benchmark::DataRecord Benchmark::ZP_DATA("ZP_DATA");