#ifndef FSS_H_
#define FSS_H_

#include "libdpf/libdpf.h"
#include "typedef.h"
#include "util.h"

class FSS1Bit {
private:
    AES_KEY aes_key_;

public:
    FSS1Bit();
    uint Gen(uint64_t index, uint64_t log_n, uchar *keys[2]);
    void EvalAll(const uchar *key, uint64_t log_n, uchar *out);
    void PseudoGen(CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prg, uint64_t index, uint64_t byte_length, uchar *dpf_out);
    bool PseudoEval(uint64_t index, uchar *dpf_out);
};

#endif /* FSS_H_ */
