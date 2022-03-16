#ifndef FSS_H_
#define FSS_H_

#include <vector>

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
    std::vector<BinaryData> Gen(uint64_t index, const uint log_n, const bool is_symmetric, bool &is_0);
    bool Eval(BinaryData &query, uint64_t index);
    void EvalAll(BinaryData &query_23, const uint log_n, std::vector<bool> &dpf_out);

    std::vector<BinaryData> PseudoGen(Peer peer[2], const uint index, const uint byte_length, const bool is_symmetric, bool &is_0);
    bool PseudoEval(BinaryData &query, const uint index);
    void PseudoEvalAll(BinaryData &query, const uint n, std::vector<bool> &dpf_out);
};

#endif /* FSS_H_ */
