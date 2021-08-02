#ifndef LIBDPF_LIBDPF_H_
#define LIBDPF_LIBDPF_H_

#include "aes.h"
#include "block.h"

int GEN(AES_KEY *key, uint64_t alpha, int n, unsigned char **k0,
        unsigned char **k1);
uint128 EVAL(AES_KEY *key, unsigned char *k, uint64_t x);
uint128 *EVALFULL(AES_KEY *key, const unsigned char *k);

void test_libdpf();

#endif /* LIBDPF_LIBDPF_H_ */
