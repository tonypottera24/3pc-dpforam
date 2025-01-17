#include "fss1bit.h"

const uint64_t masks[64] = {0x0000000000000001ULL, 0x0000000000000002ULL,
                            0x0000000000000004ULL, 0x0000000000000008ULL, 0x0000000000000010ULL,
                            0x0000000000000020ULL, 0x0000000000000040ULL, 0x0000000000000080ULL,
                            0x0000000000000100ULL, 0x0000000000000200ULL, 0x0000000000000400ULL,
                            0x0000000000000800ULL, 0x0000000000001000ULL, 0x0000000000002000ULL,
                            0x0000000000004000ULL, 0x0000000000008000ULL, 0x0000000000010000ULL,
                            0x0000000000020000ULL, 0x0000000000040000ULL, 0x0000000000080000ULL,
                            0x0000000000100000ULL, 0x0000000000200000ULL, 0x0000000000400000ULL,
                            0x0000000000800000ULL, 0x0000000001000000ULL, 0x0000000002000000ULL,
                            0x0000000004000000ULL, 0x0000000008000000ULL, 0x0000000010000000ULL,
                            0x0000000020000000ULL, 0x0000000040000000ULL, 0x0000000080000000ULL,
                            0x0000000100000000ULL, 0x0000000200000000ULL, 0x0000000400000000ULL,
                            0x0000000800000000ULL, 0x0000001000000000ULL, 0x0000002000000000ULL,
                            0x0000004000000000ULL, 0x0000008000000000ULL, 0x0000010000000000ULL,
                            0x0000020000000000ULL, 0x0000040000000000ULL, 0x0000080000000000ULL,
                            0x0000100000000000ULL, 0x0000200000000000ULL, 0x0000400000000000ULL,
                            0x0000800000000000ULL, 0x0001000000000000ULL, 0x0002000000000000ULL,
                            0x0004000000000000ULL, 0x0008000000000000ULL, 0x0010000000000000ULL,
                            0x0020000000000000ULL, 0x0040000000000000ULL, 0x0080000000000000ULL,
                            0x0100000000000000ULL, 0x0200000000000000ULL, 0x0400000000000000ULL,
                            0x0800000000000000ULL, 0x1000000000000000ULL, 0x2000000000000000ULL,
                            0x4000000000000000ULL, 0x8000000000000000ULL};

void to_byte_vector(uint64_t input, uchar *output, uint size) {
#pragma omp simd aligned(output, masks : 16)
    for (uint i = 0; i < size; i++) {
        output[i] = (input & masks[i]) != 0ULL;
    }
}

void to_byte_vector(uint128 input, uchar *output) {
    uint64_t *val = (uint64_t *)&input;
    to_byte_vector(val[0], output, 64);
    to_byte_vector(val[1], output + 64, 64);
}

FSS1Bit::FSS1Bit() {
    uint64_t userkey1 = 597349ULL;
    uint64_t userkey2 = 121379ULL;
    uint128 userkey = dpf_make_block(userkey1, userkey2);
    uchar seed[] = "abcdefghijklmnop";
    dpf_seed((uint128 *)seed);
    AES_set_encrypt_key(userkey, &aes_key_);
}

bool FSS1Bit::Gen(Peer peer[2], uint64_t index, const uint log_n, const bool is_symmetric, bool send_only, BinaryData query_23[2], Benchmark::Record *benchmark) {
#ifdef BENCHMARK_DPF
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::DPF_GEN.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    uchar *query_23_bytes[2];
    uint query_size = GEN(&aes_key_, index, log_n, &query_23_bytes[0], &query_23_bytes[1]);
    for (uint b = 0; b < 2; b++) {
        query_23[b].Resize(query_size);
        query_23[b].LoadBuffer(query_23_bytes[b]);
        delete[] query_23_bytes[b];
    }
    bool is_0 = false;
    if (!is_symmetric) {
        is_0 = this->Eval(query_23[0], index, benchmark);
    }
    if (send_only) {
        peer[0].WriteUInt(query_23[0].Size(), benchmark);
        peer[1].WriteUInt(query_23[1].Size(), benchmark);
    }
    peer[0].WriteData(query_23[0], benchmark);
    peer[1].WriteData(query_23[1], benchmark);
    if (!send_only) {
        peer[0].ReadData(query_23[1], benchmark);
        peer[1].ReadData(query_23[0], benchmark);
    }
#ifdef BENCHMARK_DPF
    if (benchmark != NULL) {
        Benchmark::DPF_GEN.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
    return is_0;
}

bool FSS1Bit::Eval(BinaryData &query, uint64_t index, Benchmark::Record *benchmark) {
#ifdef BENCHMARK_DPF
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::DPF_EVAL.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    uint128 dpf_out = EVAL(&aes_key_, query.DumpVector().data(), index);
    uint64_t index_mod = index % 128;
    uint64_t *val = (uint64_t *)&dpf_out;
    bool result = (1ll << (index_mod % 64)) & val[index_mod / 64];
#ifdef BENCHMARK_DPF
    if (benchmark != NULL) {
        Benchmark::DPF_EVAL.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
    return result;
}

void FSS1Bit::EvalAll(BinaryData &query, const uint log_n, std::vector<uchar> &dpf_out, Benchmark::Record *benchmark) {
#ifdef BENCHMARK_DPF
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::DPF_EVAL_ALL.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    uint n = 1 << log_n;
    uint128 *res = EVALFULL(&aes_key_, query.DumpVector().data());
    if (log_n <= 6) {
        to_byte_vector(((uint64_t *)res)[0], dpf_out.data(), n);
    } else {
        uint max_layer = std::max((int)log_n - 7, 0);
        uint64_t groups = 1ULL << max_layer;
        for (uint64_t i = 0; i < groups; i++) {
            to_byte_vector(res[i], dpf_out.data() + (i << 7));
        }
    }
    free(res);
#ifdef BENCHMARK_DPF
    if (benchmark != NULL) {
        Benchmark::DPF_EVAL_ALL.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
}

bool FSS1Bit::PseudoGen(Peer peer[2], const uint index, const uint byte_length, const bool is_symmetric, BinaryData query_23[2], Benchmark::Record *benchmark) {
#ifdef BENCHMARK_PSEUDO_DPF
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::PSEUDO_DPF_GEN.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    for (uint b = 0; b < 2; b++) {
        query_23[b].Resize(byte_length);
        query_23[b].Random(peer[1 - b].PRG());
    }
    std::vector<uchar> dpf_out = query_23[0].DumpVector();
    uint index_byte = index >> 3;
    uint index_bit = index & 7;
    dpf_out[index_byte] ^= 1 << index_bit;
    query_23[0].LoadBuffer(dpf_out.data());
    bool is_0 = false;
    if (!is_symmetric) {
        is_0 = dpf_out[index_byte] & (1 << index_bit);
    }
    peer[0].WriteData(query_23[0], benchmark);
    peer[1].ReadData(query_23[0], benchmark);
#ifdef BENCHMARK_PSEUDO_DPF
    if (benchmark != NULL) {
        Benchmark::PSEUDO_DPF_GEN.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
    return is_0;
}

bool FSS1Bit::PseudoEval(BinaryData &query, const uint index, Benchmark::Record *benchmark) {
#ifdef BENCHMARK_PSEUDO_DPF
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::PSEUDO_DPF_EVAL.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    bool result = get_buffer_bit(query.DumpVector().data(), index);
#ifdef BENCHMARK_PSEUDO_DPF
    if (benchmark != NULL) {
        Benchmark::PSEUDO_DPF_EVAL.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
    return result;
}

void FSS1Bit::PseudoEvalAll(BinaryData &query, const uint n, std::vector<uchar> &dpf_out, Benchmark::Record *benchmark) {
#ifdef BENCHMARK_PSEUDO_DPF
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::PSEUDO_DPF_EVAL_ALL.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    // uint index_byte = 0, index_bit = 0;
    std::vector<uchar> query_dump = query.DumpVector();
    for (uint i = 0; i < n; i++) {
        dpf_out[i] = get_buffer_bit(query_dump.data(), i);
    }
#ifdef BENCHMARK_PSEUDO_DPF
    if (benchmark != NULL) {
        Benchmark::PSEUDO_DPF_EVAL_ALL.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
}