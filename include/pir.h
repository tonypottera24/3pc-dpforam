#ifndef PIR_H_
#define PIR_H_

#include <openssl/evp.h>

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
        return inv_gadget::Inv(peer, is_0, v_sum, benchmark);
    }
}

template <typename K>
uint DPF_KEY_PIR(uint party, Peer peer[2], FSS1Bit &fss, std::vector<K> &key_array_13, const K key_23[2], Benchmark::Record *benchmark) {
    uint n = key_array_13.size();
    debug_print("[%u]DPF_KEY_PIR\n", n);

    const uint digest_size = 2;
    const uint digest_size_log = digest_size * 8;
    const uint digest_n = 1 << digest_size_log;
    const EVP_MD *evp_sha256 = EVP_sha256();
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    uchar *sha256_digest = (unsigned char *)OPENSSL_malloc(EVP_MD_size(evp_sha256));
    uint sha256_digest_size;

    uint v_sum = 0;
    if (party == 2) {
        const uint P0 = 1;
        K key = key_23[0] + key_23[1];
        key.Print("key");
        std::vector<uchar> key_dump = key.Dump();
        print_bytes(key_dump.data(), key_dump.size(), "key_dump");

        for (uint b = 0; b < 2; b++) {
            if (b == 1) {
                key_dump[0] ^= 1;
            }

            EVP_DigestInit_ex2(md_ctx, evp_sha256, NULL);
            EVP_DigestUpdate(md_ctx, key_dump.data(), key_dump.size());
            EVP_DigestFinal_ex(md_ctx, sha256_digest, &sha256_digest_size);
            // fprintf(stderr, "sha256_digest_size = %u\n", sha256_digest_size);
            uint digest_uint;
            // memcpy(&digest_uint, sha256_digest, digest_size);
            memcpy(&digest_uint, sha256_digest, sizeof(uint));
            digest_uint %= digest_n;

            // print_bytes(digest, digest_size, "digest");

            BinaryData query_23[2];
            bool is_0 = false;

            fss.Gen(digest_uint, digest_size_log, true, query_23, is_0);

            peer[0].WriteUInt(query_23[0].Size(), benchmark);
            peer[1].WriteUInt(query_23[1].Size(), benchmark);

            peer[0].WriteData(query_23[0], benchmark);
            peer[1].WriteData(query_23[1], benchmark);
        }
        v_sum = rand_uint(peer[P0].PRG()) % n;
    } else {  // party == 0 || party == 1
        std::vector<uchar> key_dump_array[n];
        uint key_array_digest[n];
        std::unordered_map<uint, uint> exists;
        uint collision_ct = 0;

        for (uint i = 0; i < n; i++) {
            K key = key_array_13[i] - key_23[1 - party];
            key_dump_array[i] = key.Dump();

            EVP_DigestInit_ex2(md_ctx, evp_sha256, NULL);
            EVP_DigestUpdate(md_ctx, key_dump_array[i].data(), key_dump_array[i].size());
            EVP_DigestFinal_ex(md_ctx, sha256_digest, &sha256_digest_size);

            uint digest_uint;
            // memcpy(&digest_uint, sha256_digest, digest_size);
            memcpy(&digest_uint, sha256_digest, sizeof(uint));
            digest_uint %= digest_n;
            key_array_digest[i] = digest_uint;

            if (exists.find(digest_uint) == exists.end()) {
                exists[digest_uint] = 1;
            } else {
                exists[digest_uint]++;
                collision_ct++;
            }
        }

        // for (uint i = 0; i < n; i++) {
        //     uint key_digest = key_array_digest[i];
        //     uint ct = exists[key_digest];
        //     // K key = key_array_13[i] - key_23[1 - party];
        //     fprintf(stderr, "exists, i = %u, key_digest = %u, ct = %u\n", i, key_digest, ct);
        // }

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

        for (uint b = 0; b < 2; b++) {
            if (b == 0 || collision_mode) {
                dpf_out[b].resize(digest_n);
                fss.EvalAll(query[b], digest_size_log, dpf_out[b]);
            }
        }

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
                key_dump_array[i][0] ^= 1;
                EVP_DigestInit_ex2(md_ctx, evp_sha256, NULL);
                EVP_DigestUpdate(md_ctx, key_dump_array[i].data(), key_dump_array[i].size());
                EVP_DigestFinal_ex(md_ctx, sha256_digest, &sha256_digest_size);

                uint digest_uint;
                // memcpy(&digest_uint, sha256_digest, digest_size);
                memcpy(&digest_uint, sha256_digest, sizeof(uint));
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

    EVP_MD_CTX_free(md_ctx);
    OPENSSL_free(sha256_digest);

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