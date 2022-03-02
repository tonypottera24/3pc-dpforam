#include "libdpf.h"

#include <omp.h>
#include <stdio.h>
#include <string.h>

#include "block.h"

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
    int lsb = dpf_lsb(input);

    if (lsb == 1) {
        return dpf_reverse_lsb(input);
    } else {
        return input;
    }
}

int getbit(const uint64_t x, const uint64_t n, const uint64_t b) {
    return (x >> (n - b)) & 1;
}

void PRG(AES_KEY *key, uint128 input, uint128 *output1, uint128 *output2, int *bit1,
         int *bit2) {
    input = dpf_set_lsb_zero(input);

    uint128 stash[2];
    stash[0] = input;
    stash[1] = dpf_reverse_lsb(input);

    AES_ecb_encrypt_blks(stash, 2, key);

    stash[0] = uint128_xor(stash[0], input);
    stash[1] = uint128_xor(stash[1], input);
    stash[1] = dpf_reverse_lsb(stash[1]);

    *bit1 = dpf_lsb(stash[0]);
    *bit2 = dpf_lsb(stash[1]);

    *output1 = dpf_set_lsb_zero(stash[0]);
    *output2 = dpf_set_lsb_zero(stash[1]);
}

int GEN(AES_KEY *key, uint64_t alpha, const uint log_n, uchar **k0, uchar **k1) {
    uint maxlayer = max((int)log_n - 7, 0);
    // int maxlayer = n;

    uint128 s[maxlayer + 1][2];
    int t[maxlayer + 1][2];
    uint128 sCW[maxlayer];
    int tCW[maxlayer][2];

    s[0][0] = dpf_random_block();
    s[0][1] = dpf_random_block();
    t[0][0] = dpf_lsb(s[0][0]);
    t[0][1] = t[0][0] ^ 1;
    s[0][0] = dpf_set_lsb_zero(s[0][0]);
    s[0][1] = dpf_set_lsb_zero(s[0][1]);

    uint128 s0[2], s1[2];  // 0=L,1=R
#define LEFT 0
#define RIGHT 1
    int t0[2], t1[2];
    for (uint i = 1; i <= maxlayer; i++) {
        PRG(key, s[i - 1][0], &s0[LEFT], &s0[RIGHT], &t0[LEFT], &t0[RIGHT]);
        PRG(key, s[i - 1][1], &s1[LEFT], &s1[RIGHT], &t1[LEFT], &t1[RIGHT]);

        int keep, lose;
        int alphabit = getbit(alpha, log_n, i);
        if (alphabit == 0) {
            keep = LEFT;
            lose = RIGHT;
        } else {
            keep = RIGHT;
            lose = LEFT;
        }

        sCW[i - 1] = uint128_xor(s0[lose], s1[lose]);

        tCW[i - 1][LEFT] = t0[LEFT] ^ t1[LEFT] ^ alphabit ^ 1;
        tCW[i - 1][RIGHT] = t0[RIGHT] ^ t1[RIGHT] ^ alphabit;

        if (t[i - 1][0] == 1) {
            s[i][0] = uint128_xor(s0[keep], sCW[i - 1]);
            t[i][0] = t0[keep] ^ tCW[i - 1][keep];
        } else {
            s[i][0] = s0[keep];
            t[i][0] = t0[keep];
        }

        if (t[i - 1][1] == 1) {
            s[i][1] = uint128_xor(s1[keep], sCW[i - 1]);
            t[i][1] = t1[keep] ^ tCW[i - 1][keep];
        } else {
            s[i][1] = s1[keep];
            t[i][1] = t1[keep];
        }
    }

    uint128 finalblock;
    finalblock = dpf_zero_block();
    finalblock = dpf_reverse_lsb(finalblock);

    char shift = (alpha)&127;
    // char shift = alpha[0];
    if (shift & 64) {
        finalblock = dpf_left_shirt(finalblock, 64);
    }
    if (shift & 32) {
        finalblock = dpf_left_shirt(finalblock, 32);
    }
    if (shift & 16) {
        finalblock = dpf_left_shirt(finalblock, 16);
    }
    if (shift & 8) {
        finalblock = dpf_left_shirt(finalblock, 8);
    }
    if (shift & 4) {
        finalblock = dpf_left_shirt(finalblock, 4);
    }
    if (shift & 2) {
        finalblock = dpf_left_shirt(finalblock, 2);
    }
    if (shift & 1) {
        finalblock = dpf_left_shirt(finalblock, 1);
    }
    // dpf_cb(finalblock);
    finalblock = dpf_reverse_lsb(finalblock);

    finalblock = uint128_xor(finalblock, s[maxlayer][0]);
    finalblock = uint128_xor(finalblock, s[maxlayer][1]);

    uchar *buff0;
    uchar *buff1;
    int size = 1 + 16 + 1 + 18 * maxlayer + 16;
    //	buff0 = (uchar*) malloc(size);
    //	buff1 = (uchar*) malloc(size);
    buff0 = new uchar[size];
    buff1 = new uchar[size];

    if (buff0 == NULL || buff1 == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    buff0[0] = log_n;
    memcpy(&buff0[1], &s[0][0], 16);
    buff0[17] = t[0][0];
    for (uint i = 1; i <= maxlayer; i++) {
        memcpy(&buff0[18 * i], &sCW[i - 1], 16);
        buff0[18 * i + 16] = tCW[i - 1][0];
        buff0[18 * i + 17] = tCW[i - 1][1];
    }
    memcpy(&buff0[18 * maxlayer + 18], &finalblock, 16);

    buff1[0] = log_n;
    memcpy(&buff1[18], &buff0[18], 18 * (maxlayer));
    memcpy(&buff1[1], &s[0][1], 16);
    buff1[17] = t[0][1];
    memcpy(&buff1[18 * maxlayer + 18], &finalblock, 16);

    *k0 = buff0;
    *k1 = buff1;

    return size;
}

uint128 EVAL(AES_KEY *key, uchar *k, uint64_t x) {
    int log_n = k[0];
    int max_layer = max(log_n - 7, 0);

    uint128 s[max_layer + 1];
    int t[max_layer + 1];
    uint128 sCW[max_layer];
    int tCW[max_layer][2];
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
    int tL, tR;
    for (uint i = 1; i <= max_layer; i++) {
        PRG(key, s[i - 1], &sL, &sR, &tL, &tR);

        if (t[i - 1] == 1) {
            sL = uint128_xor(sL, sCW[i - 1]);
            sR = uint128_xor(sR, sCW[i - 1]);
            tL = tL ^ tCW[i - 1][0];
            tR = tR ^ tCW[i - 1][1];
        }

        int xbit = getbit(x, log_n, i);
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

uint128 *EVALFULL(AES_KEY *key, const uchar *k) {
    int n = k[0];
    int maxlayer = max(n - 7, 0);
    uint64_t maxlayeritem = 1 << maxlayer;

    // block s[2][maxlayeritem];
    // int t[2][maxlayeritem];
    uint128 *s[2];
    int *t[2];
    for (int b = 0; b < 2; b++) {
        s[b] = new uint128[maxlayeritem];
        t[b] = new int[maxlayeritem];
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

    // block sL, sR;
    // int tL, tR;
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

// void test_libdpf() {
//     uint64_t userkey1 = 597349;
//     uint64_t userkey2 = 121379;
//     uint128 userkey = dpf_make_block(userkey1, userkey2);

//     dpf_seed(NULL);

//     AES_KEY key;
//     AES_set_encrypt_key(userkey, &key);

//     uchar *k0;
//     uchar *k1;

//     GEN(&key, 1, 1, &k0, &k1);

//     uint128 res1;
//     uint128 res2;

//     res1 = EVAL(&key, k0, 0);
//     res2 = EVAL(&key, k1, 0);
//     dpf_cb(res1);
//     dpf_cb(res2);
//     dpf_cb(uint128_xor(res1, res2));

//     res1 = EVAL(&key, k0, 128);
//     res2 = EVAL(&key, k1, 128);
//     dpf_cb(res1);
//     dpf_cb(res2);
//     dpf_cb(uint128_xor(res1, res2));

//     uint128 *resf0, *resf1;
//     resf0 = EVALFULL(&key, k0);
//     resf1 = EVALFULL(&key, k1);

//     for (uint64_t j = 0; j < 1; j++) {
//         fprintf(stderr, "Group %llu\n", j);

//         dpf_cb(resf0[j]);
//         dpf_cb(resf1[j]);
//         dpf_cb(uint128_xor(resf0[j], resf1[j]));
//     }
// }
