#ifndef FSS_H_
#define FSS_H_

#include "binary_data.h"
#include "libdpf/libdpf.h"
#include "peer.h"
#include "typedef.h"
#include "util.h"

class FSS1Bit {
private:
    AES_KEY aes_key_;

public:
    FSS1Bit();
    std::pair<std::vector<BinaryData>, bool> Gen(uint64_t index, uint64_t log_n, const bool is_symmetric);
    bool Eval(BinaryData &query, uint64_t index);
    uchar *EvalAll(BinaryData &query_23, uint64_t log_n);
    std::pair<std::vector<BinaryData>, bool> PseudoGen(Peer peer[2], uint64_t index, uint64_t byte_length, const bool is_symmetric);
    bool PseudoEval(BinaryData &query, const uint64_t index);
    bool *PseudoEvalAll(BinaryData &query, const uint64_t n);
};

#endif /* FSS_H_ */
