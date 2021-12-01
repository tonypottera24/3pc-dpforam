#ifndef LIBDPF_H
#define LIBDPF_H

#include <inttypes.h>

#include <cstring>
#include <vector>

#include "aes.h"
#include "block.h"
#include "dpf_key.h"

#define LEFT 0
#define RIGHT 1

#define max(a, b)                   \
    (                               \
        {                           \
            __typeof__(a) _a = (a); \
            __typeof__(b) _b = (b); \
            _a > _b ? _a : _b;      \
        })

uint128 dpf_reverse_lsb(uint128 input) {
    static uint64_t b1 = 0;
    static uint64_t b2 = 1;
    uint128 _xor = dpf_make_block(b1, b2);
    return uint128_xor(input, _xor);
}

uint128 dpf_set_lsb_zero(uint128 input) {
    bool lsb = dpf_lsb(input);

    if (lsb) {
        return dpf_reverse_lsb(input);
    } else {
        return input;
    }
}

void PRG(AES_KEY *key, uint128 input, uint128 &output1, uint128 &output2, bool &bit1,
         bool &bit2) {
    input = dpf_set_lsb_zero(input);

    uint128 stash[2];
    stash[0] = input;
    stash[1] = dpf_reverse_lsb(input);

    AES_ecb_encrypt_blks(stash, 2, key);

    stash[0] = uint128_xor(stash[0], input);
    stash[1] = uint128_xor(stash[1], input);
    stash[1] = dpf_reverse_lsb(stash[1]);

    bit1 = dpf_lsb(stash[0]);
    bit2 = dpf_lsb(stash[1]);

    output1 = dpf_set_lsb_zero(stash[0]);
    output2 = dpf_set_lsb_zero(stash[1]);
}

bool getbit(uint64_t x, int n, int b) {
    return (x >> (n - b)) & 1;
}

template <typename D>
DPFKey<D> *GEN(AES_KEY *key, uint64_t alpha, D beta, int n) {
    uint128 s[2];
    bool t[2];
    DPFKey<D> *dpf_key = new DPFKey<D>[2];

    s[0] = dpf_random_block();
    s[1] = dpf_random_block();
    t[0] = dpf_lsb(s[0]);
    t[1] = t[0][0] ^ 1;
    s[0] = dpf_set_lsb_zero(s[0]);
    s[1] = dpf_set_lsb_zero(s[1]);

    for (uint b = 0; b < 2; b++) {
        dpf_key[b].s0 = s[b];
        dpf_key[b].t0 = t[b];
    }

    for (uint i = 1; i <= n; i++) {
        uint128 ss[2][2];  // 0=L, 1=R
        bool tt[2][2];

        for (uint b = 0; b < 2; b++) {
            PRG(key, s[b], ss[b][LEFT], ss[b][RIGHT], tt[b][LEFT], tt[b][RIGHT]);
        }

        bool alpha_i = getbit(alpha, n, i);
        bool keep, lose;
        if (alpha_i == false) {
            keep = LEFT;
            lose = RIGHT;
        } else {
            keep = RIGHT;
            lose = LEFT;
        }

        uint128 sCW = uint128_xor(ss[0][lose], ss[1][lose]);

        bool tCW[2];
        tCW[LEFT] = tt[0][LEFT] ^ tt[1][LEFT] ^ alpha_i ^ 1;
        tCW[RIGHT] = tt[0][RIGHT] ^ tt[1][RIGHT] ^ alpha_i;

        for (uint b = 0; b < 2; b++) {
            dpf_key[b].sCW.push_back(sCW);
            dpf_key[b].tCW[LEFT].push_back(tCW[LEFT]);
            dpf_key[b].tCW[RIGHT].push_back(tCW[RIGHT]);
            if (t[b]) {
                s[b] = uint128_xor(ss[b][keep], sCW);
                t[b] = tt[b][keep] ^ tCW[keep];
            } else {
                s[b] = ss[b][keep];
                t[b] = tt[b][keep];
            }
        }
    }
    D cs[2];
    for (uint b = 0; b < 2; b++) {
        cs[b].Load(s[b], beta.Size());
    }
    if (t[1]) {
        dpf_key->last_CW = -(beta -);
    } else {
        dpf_key->last_CW = ;
    }

    return dpf_key;
}

template <typename D>
uint128 EVAL(AES_KEY *key, unsigned char *k, uint64_t x) {
    int n = k[0];
    int max_layer = max(n - 7, 0);

    uint128 s[max_layer + 1];
    bool t[max_layer + 1];
    uint128 sCW[max_layer];
    bool tCW[max_layer][2];
    uint128 final_block;

    memcpy(&s[0], &k[1], 16);
    t[0] = k[17];

    for (uint i = 1; i <= max_layer; i++) {
        memcpy(&sCW[i - 1], &k[18 * i], 16);
        tCW[i - 1][0] = k[18 * i + 16];
        tCW[i - 1][1] = k[18 * i + 17];
    }

    memcpy(&final_block, &k[18 * (max_layer + 1)], 16);

    uint128 sL, sR;
    bool tL, tR;
    for (uint i = 1; i <= max_layer; i++) {
        PRG(key, s[i - 1], &sL, &sR, &tL, &tR);

        if (t[i - 1] == 1) {
            sL = uint128_xor(sL, sCW[i - 1]);
            sR = uint128_xor(sR, sCW[i - 1]);
            tL = tL ^ tCW[i - 1][0];
            tR = tR ^ tCW[i - 1][1];
        }

        bool xbit = getbit(x, n, i);
        if (xbit == 0) {
            s[i] = sL;
            t[i] = tL;
        } else {
            s[i] = sR;
            t[i] = tR;
        }
    }

    uint128 res;
    res = s[max_layer];
    if (t[max_layer] == 1) {
        res = dpf_reverse_lsb(res);
    }

    if (t[max_layer] == 1) {
        res = uint128_xor(res, final_block);
    }

    return res;
}

template <typename D>
D *EVALFULL(AES_KEY *key, const unsigned char *k) {
    int n = k[0];
    int maxlayer = max(n - 7, 0);
    uint64_t maxlayeritem = 1 << maxlayer;

    //block s[2][maxlayeritem];
    //int t[2][maxlayeritem];
    uint128 *s[2];
    int *t[2];
    for (int ii = 0; ii < 2; ii++) {
        s[ii] = new uint128[maxlayeritem];
        t[ii] = new int[maxlayeritem];
    }

    int curlayer = 1;

    uint128 sCW[maxlayer];
    int tCW[maxlayer][2];
    uint128 finalblock;

    memcpy(&s[0][0], &k[1], 16);
    t[0][0] = k[17];

    for (uint i = 1; i <= maxlayer; i++) {
        memcpy(&sCW[i - 1], &k[18 * i], 16);
        tCW[i - 1][0] = k[18 * i + 16];
        tCW[i - 1][1] = k[18 * i + 17];
    }

    memcpy(&finalblock, &k[18 * (maxlayer + 1)], 16);

    //block sL, sR;
    //int tL, tR;
    for (uint i = 1; i <= maxlayer; i++) {
        uint64_t itemnumber = 1 << (i - 1);
        //#pragma omp parallel for
        for (uint64_t j = 0; j < itemnumber; j++) {
            uint128 sL, sR;
            int tL, tR;
            PRG(key, s[1 - curlayer][j], &sL, &sR, &tL, &tR);

            if (t[1 - curlayer][j] == 1) {
                sL = uint128_xor(sL, sCW[i - 1]);
                sR = uint128_xor(sR, sCW[i - 1]);
                tL = tL ^ tCW[i - 1][0];
                tR = tR ^ tCW[i - 1][1];
            }

            s[curlayer][2 * j] = sL;
            t[curlayer][2 * j] = tL;
            s[curlayer][2 * j + 1] = sR;
            t[curlayer][2 * j + 1] = tR;
        }
        curlayer = 1 - curlayer;
    }

    uint64_t itemnumber = 1 << maxlayer;
    uint128 *res = (uint128 *)malloc(sizeof(uint128) * itemnumber);

    //#pragma omp parallel for
    for (uint64_t j = 0; j < itemnumber; j++) {
        res[j] = s[1 - curlayer][j];

        if (t[1 - curlayer][j] == 1) {
            res[j] = dpf_reverse_lsb(res[j]);
        }

        if (t[1 - curlayer][j] == 1) {
            res[j] = uint128_xor(res[j], finalblock);
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] s[b];
        delete[] t[b];
    }

    return res;
}

template <typename D>
void test_libdpf() {
    uint64_t userkey1 = 597349;
    uint64_t userkey2 = 121379;
    uint128 userkey = dpf_make_block(userkey1, userkey2);

    dpf_seed(NULL);

    AES_KEY key;
    AES_set_encrypt_key(userkey, &key);

    unsigned char *k0;
    unsigned char *k1;

    GEN(&key, 1, 1, &k0, &k1);

    uint128 res1;
    uint128 res2;

    res1 = EVAL(&key, k0, 0);
    res2 = EVAL(&key, k1, 0);
    dpf_cb(res1);
    dpf_cb(res2);
    dpf_cb(uint128_xor(res1, res2));

    res1 = EVAL(&key, k0, 128);
    res2 = EVAL(&key, k1, 128);
    dpf_cb(res1);
    dpf_cb(res2);
    dpf_cb(uint128_xor(res1, res2));

    uint128 *resf0, *resf1;
    resf0 = EVALFULL(&key, k0);
    resf1 = EVALFULL(&key, k1);

    for (uint64_t j = 0; j < 1; j++) {
        fprintf(stderr, "Group %llu\n", j);

        dpf_cb(resf0[j]);
        dpf_cb(resf1[j]);
        dpf_cb(uint128_xor(resf0[j], resf1[j]));
    }
}

#endif