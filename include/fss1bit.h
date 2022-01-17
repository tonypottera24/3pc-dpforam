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
    std::pair<std::vector<BinaryData>, bool> Gen(uchar *index, const uint log_n, const bool is_symmetric);
    bool Eval(BinaryData &query, uchar *index);
    std::vector<bool> EvalAll(BinaryData &query_23, const uint log_n);

    std::pair<std::vector<BinaryData>, bool> PseudoGen(Peer peer[2], const uint index, const uint byte_length, const bool is_symmetric);
    bool PseudoEval(BinaryData &query, const uint index);
    std::vector<bool> PseudoEvalAll(BinaryData &query, const uint n);
};

#endif /* FSS_H_ */
