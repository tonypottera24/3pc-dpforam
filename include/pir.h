#ifndef PIR_H_
#define PIR_H_

#include <openssl/evp.h>

#include <algorithm>
#include <string>
#include <unordered_map>

#include "ssot.h"

namespace PIR {

template <typename D>
D DPF_PIR(Peer peer[2], FSS1Bit &fss, std::vector<D> array_23[2], const uint n, const uint log_n, const uint index_23[2], bool pseudo, Benchmark::Record *benchmark) {
    debug_print("[%u]DPF_PIR, index_23 = (%u, %u), log_n = %u\n", n, index_23[0], index_23[1], log_n);
    // fprintf(stderr, "[%u]DPF_PIR, index_23 = (%u, %u), log_n = %u\n", n, index_23[0], index_23[1], log_n);
    // only accept power of 2 n
    // fprintf(stderr, "[%u]DPF_PIR, index_23 = (%u, %u), log_n = %u\n", n, index_23[0], index_23[1], log_n);

    const bool is_symmetric = array_23[0][0].IsSymmetric();

    BinaryData query_23[2];
    bool is_0 = false;
    if (pseudo) {
        uint data_length = divide_ceil(n, 8);
        fss.PseudoGen(peer, index_23[0] ^ index_23[1], data_length, is_symmetric, query_23, is_0);
        peer[0].WriteData(query_23[0], benchmark);
        peer[1].ReadData(query_23[0]);
    } else {
        fss.Gen(index_23[0] ^ index_23[1], log_n, is_symmetric, query_23, is_0);

        peer[0].WriteData(query_23[0], benchmark);
        peer[1].WriteData(query_23[1], benchmark);

        peer[0].ReadData(query_23[1]);
        peer[1].ReadData(query_23[0]);
    }

    uint data_size = array_23[0][0].Size();
    D v_sum[2] = {D(data_size), D(data_size)};
    std::vector<uchar> dpf_out(n);

    for (uint b = 0; b < 2; b++) {
        if (pseudo) {
            fss.PseudoEvalAll(query_23[b], n, dpf_out);
        } else {
            fss.EvalAll(query_23[b], log_n, dpf_out);
        }
        for (uint i = 0; i < array_23[0].size(); i++) {
            // debug_print("[%u]DPF_PIR, i = %u, ii = %u, dpf_out = %u\n", n, i, i ^ index_23[b], (uint)dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                v_sum[b] += array_23[b][i];
            }
        }
    }

    if (is_symmetric) {
        return v_sum[0] + v_sum[1];
    } else {
#ifdef BENCHMARK_GROUP_PREPARE
        Benchmark::GROUP_PREPARE_READ.Start();
#endif
        D ans = inv_gadget::Inv(peer, is_0, v_sum, benchmark);
#ifdef BENCHMARK_GROUP_PREPARE
        Benchmark::GROUP_PREPARE_READ.End();
#endif
        return ans;
    }
}

template <typename K>
uint DPF_KEY_PIR(uint party, Peer peer[2], FSS1Bit &fss, std::vector<K> &key_array_13, const K key_23[2], Benchmark::Record *benchmark) {
    uint n = key_array_13.size();
    debug_print("[%u]DPF_KEY_PIR\n", n);

    const uint digest_size = 2;
    const uint digest_size_log = digest_size * 8;
    const uint digest_n = 1 << digest_size_log;

    EVP_CIPHER_CTX *cipher_ctx = EVP_CIPHER_CTX_new();
    const EVP_CIPHER *evp_cipher = EVP_aes_128_ecb();
    uint aes_block_size = EVP_CIPHER_get_block_size(evp_cipher);
    uchar *aes_key[2] = {(uchar *)"01234567890123456789012345678901", (uchar *)"01234567890123456789012345678902"};
    uchar *aes_iv[2] = {(uchar *)"0123456789012345", (uchar *)"0123456789012346"};
    uchar *aes_block = (unsigned char *)OPENSSL_malloc(aes_block_size);

    // AES_KEY aes_key[2];
    // uint128 userkey = _mm_set_epi64((__m64)597349ULL, (__m64)121379ULL);
    // AES_set_encrypt_key(userkey, &aes_key[0]);
    // userkey = _mm_set_epi64((__m64)121379ULL, (__m64)597349ULL);
    // AES_set_encrypt_key(userkey, &aes_key[1]);
    // std::vector<uchar> aes_block;

    uint v_sum = 0;
    if (party == 2) {
        const uint P0 = 1;
        K key = key_23[0] + key_23[1];
        key.Print("key");
        std::vector<uchar> key_dump = key.Dump();
        print_bytes(key_dump.data(), key_dump.size(), "key_dump");

        for (uint b = 0; b < 2; b++) {
#ifdef BENCHMARK_KEY_VALUE_HASH
            Benchmark::KEY_VALUE_HASH.Start();
#endif
            int aes_block_size;
            EVP_EncryptInit_ex2(cipher_ctx, evp_cipher, aes_key[b], aes_iv[b], NULL);
            EVP_EncryptUpdate(cipher_ctx, aes_block, &aes_block_size, key_dump.data(), key_dump.size());
            EVP_EncryptFinal_ex(cipher_ctx, aes_block, &aes_block_size);

            // aes_block = hash(key_dump, sizeof(uint), aes_key[b]);
#ifdef BENCHMARK_KEY_VALUE_HASH
            Benchmark::KEY_VALUE_HASH.End();
#endif

            uint digest_uint;
            memcpy(&digest_uint, aes_block, std::min(sizeof(uint), (unsigned long)aes_block_size));
            // memcpy(&digest_uint, aes_block.data(), sizeof(uint));
            digest_uint %= digest_n;

            BinaryData query_23[2];
            bool is_0 = false;

            fss.Gen(digest_uint, digest_size_log, true, query_23, is_0);

            peer[0].WriteUInt(query_23[0].Size(), benchmark);
            peer[1].WriteUInt(query_23[1].Size(), benchmark);

            peer[0].WriteData(query_23[0], benchmark);
            peer[1].WriteData(query_23[1], benchmark);
        }
        v_sum = rand_uint(peer[P0].PRG()) % n;

        debug_print("GG success\n");
    } else {  // party == 0 || party == 1

        debug_print("GG start n = %u\n", n);

        std::vector<std::vector<uchar>> key_dump_array;
        std::vector<uint> key_array_digest(n);
        std::unordered_map<uint, uint> exists;
        uint collision_ct = 0;

        EVP_EncryptInit_ex2(cipher_ctx, evp_cipher, aes_key[0], aes_iv[0], NULL);

        debug_print("GG0\n");

        for (uint i = 0; i < n; i++) {
            K key = key_array_13[i] - key_23[1 - party];
            key_dump_array.push_back(key.Dump());

#ifdef BENCHMARK_KEY_VALUE_HASH
            Benchmark::KEY_VALUE_HASH.Start();
#endif
            int aes_block_size;
            EVP_EncryptInit_ex2(cipher_ctx, NULL, NULL, NULL, NULL);
            EVP_EncryptUpdate(cipher_ctx, aes_block, &aes_block_size, key_dump_array[i].data(), key_dump_array[i].size());
            EVP_EncryptFinal_ex(cipher_ctx, aes_block, &aes_block_size);

            // aes_block = hash(key_dump_array[i], sizeof(uint), aes_key[0]);
#ifdef BENCHMARK_KEY_VALUE_HASH
            Benchmark::KEY_VALUE_HASH.End();
#endif

            uint digest_uint;
            memcpy(&digest_uint, aes_block, std::min(sizeof(uint), (unsigned long)aes_block_size));
            // memcpy(&digest_uint, aes_block.data(), sizeof(uint));
            digest_uint %= digest_n;
            key_array_digest[i] = digest_uint;

            if (exists.find(digest_uint) == exists.end()) {
                exists[digest_uint] = 1;
            } else {
                exists[digest_uint]++;
                collision_ct++;
            }
        }

        debug_print("GG1\n");

        // bool collision_mode = true;
        bool collision_mode = collision_ct > 2;
        // fprintf(stderr, "collision_ct = %u\n", collision_ct);

        BinaryData query[2];
        std::vector<uchar> dpf_out[2];

        for (uint b = 0; b < 2; b++) {
            uint query_size = peer[party].ReadUInt();
            query[b].Resize(query_size);
            peer[party].ReadData(query[b]);
        }

        debug_print("GG2\n");

        for (uint b = 0; b < 2; b++) {
            if (b == 0 || collision_mode) {
                dpf_out[b].resize(digest_n);
                fss.EvalAll(query[b], digest_size_log, dpf_out[b]);
            }
        }

        EVP_EncryptInit_ex2(cipher_ctx, evp_cipher, aes_key[1], aes_iv[1], NULL);

        for (uint i = 0; i < n; i++) {
            uint key_digest = key_array_digest[i];
            if (exists[key_digest] == 1) {
                // debug_print("dpf i = %u, exists = 1\n", i);
                // print_bytes((uchar *)&key_digest, digest_size, "key_digest");
                // if (fss.Eval(query[0], key_digest)) {
                if (dpf_out[0][key_digest]) {
                    // debug_print("dpf i = %u, fss.Eval = true\n", i);
                    v_sum ^= i;
                }
            } else {  // collision
                      // fprintf(stderr, "collision, i = %u\n", i);
#ifdef BENCHMARK_KEY_VALUE_HASH
                Benchmark::KEY_VALUE_HASH.Start();
#endif
                int aes_block_size;
                EVP_EncryptInit_ex2(cipher_ctx, NULL, NULL, NULL, NULL);
                EVP_EncryptUpdate(cipher_ctx, aes_block, &aes_block_size, key_dump_array[i].data(), key_dump_array[i].size());
                EVP_EncryptFinal_ex(cipher_ctx, aes_block, &aes_block_size);

                // aes_block = hash(key_dump_array[i], sizeof(uint), aes_key[1]);
#ifdef BENCHMARK_KEY_VALUE_HASH
                Benchmark::KEY_VALUE_HASH.End();
#endif

                uint digest_uint;
                memcpy(&digest_uint, aes_block, std::min(sizeof(uint), (unsigned long)aes_block_size));
                // memcpy(&digest_uint, aes_block.data(), sizeof(uint));
                digest_uint %= digest_n;

                bool hit = false;
                if (collision_mode) {
                    hit = dpf_out[1][digest_uint];
                } else {
                    hit = fss.Eval(query[1], digest_uint);
                }
                if (hit) {
                    v_sum ^= i;
                }
                // collision can still happen...
            }
        }

        if (party == 0) {
            const uint P2 = 0;
            v_sum ^= rand_uint(peer[P2].PRG()) % n;
        }
    }

    EVP_CIPHER_CTX_free(cipher_ctx);
    OPENSSL_free(aes_block);

    return v_sum % n;
}

template <typename D>
D SSOT_PIR(uint party, Peer peer[2], std::vector<D> &array_13, const uint index_23[2], Benchmark::Record *benchmark) {
    // TODO n may not be power of 2
    uint n = array_13.size();
    debug_print("[%lu]SSOT_PIR, n = %u, index_23 = (%u, %u)\n", array_13.size(), n, index_23[0], index_23[1]);
    uint data_size = array_13[0].Size();
    D v_out_13(data_size);
    if (party == 2) {
        // const uint P1 = 0, P0 = 1;
        const uint P0 = 1;
        peer[P0].WriteDataVector(array_13, benchmark);
        SSOT::P2<D>(peer, n, data_size, benchmark);
        v_out_13.Random(peer[P0].PRG());
    } else if (party == 0) {
        const uint P2 = 0, P1 = 1;

        std::vector<D> u(n, D(data_size));
        peer[P2].ReadDataVector(u);
        for (uint i = 0; i < n; i++) {
            u[i] += array_13[i];
        }
        v_out_13 = SSOT::P0(peer, index_23[P1] ^ index_23[P2], u, benchmark);

        D tmp(data_size);
        tmp.Random(peer[P2].PRG());
        v_out_13 -= tmp;
    } else {  // this->party_ == 1
        // const uint P0 = 0, P2 = 1;
        const uint P2 = 1;
        v_out_13 = SSOT::P1(peer, index_23[P2], array_13, benchmark);
    }
    return v_out_13;
}

template <typename D>
D PIR(Peer peer[2], FSS1Bit &fss, std::vector<D> array_23[2], const uint index_23[2], Benchmark::Record *benchmark) {
    uint n = pow2_ceil(array_23[0].size());
    uint log_n = log2(n);
    uint clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%lu]PIR, n = %u\n", array_23[0].size(), n);
    if (n == 1) {
        return array_23[0][0];
    } else {
        bool pseudo = (n <= PSEUDO_DPF_THRESHOLD);
        return DPF_PIR(peer, fss, array_23, n, log_n, clean_index_23, pseudo, benchmark);
    }
}

};  // namespace PIR

#endif /* PIR_H_ */