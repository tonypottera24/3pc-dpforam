#ifndef FSS_H_
#define FSS_H_

#include "libdpf/libdpf.h"
#include "typedef.h"

class FSS1Bit {
private:
    AES_KEY aes_key_;

public:
    FSS1Bit();
    uint Gen(uint64_t alpha, uint m, uchar *keys[2]);
    void EvalAll(const uchar *key, uint m, uchar *out);
    void EvalAllWithPerm(const uchar *key, uint m, uint64_t perm, uchar *out);
};

#endif /* FSS_H_ */
