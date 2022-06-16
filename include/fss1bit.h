#ifndef FSS_H_
#define FSS_H_

#include <string.h>
#include <x86intrin.h>

#include <algorithm>
#include <vector>

#include "benchmark/constant.h"
#include "constant.h"
#include "data/binary_data.h"
#include "libdpf/libdpf.h"
#include "peer.h"
#include "typedef.h"
#include "util.h"
class FSS1Bit {
private:
    AES_KEY aes_key_;

public:
    FSS1Bit();
    bool Gen(Peer peer[2], uint64_t index, const uint log_n, const bool is_symmetric, bool send_only, BinaryData query_23[2], Benchmark::Record *benchmark);
    bool Eval(BinaryData &query, uint64_t index, Benchmark::Record *benchmark);
    void EvalAll(BinaryData &query_23, const uint log_n, std::vector<uchar> &dpf_out, Benchmark::Record *benchmark);

    bool PseudoGen(Peer peer[2], const uint index, const uint byte_length, const bool is_symmetric, BinaryData query_23[2], Benchmark::Record *benchmark);
    bool PseudoEval(BinaryData &query, const uint index, Benchmark::Record *benchmark);
    void PseudoEvalAll(BinaryData &query, const uint n, std::vector<uchar> &dpf_out, Benchmark::Record *benchmark);
};

#endif /* FSS_H_ */
