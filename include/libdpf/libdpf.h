#ifndef LIBDPF_LIBDPF_H_
#define LIBDPF_LIBDPF_H_

#include "aes.h"
#include "block.h"
#include "typedef.h"
#include "util.h"

int GEN(AES_KEY *key, uchar *alpha, const uint n, uchar **k0, uchar **k1);
uint128 EVAL(AES_KEY *key, uchar *k, uchar *x);
uint128 *EVALFULL(AES_KEY *key, const uchar *k);

void test_libdpf();

#endif /* LIBDPF_LIBDPF_H_ */
