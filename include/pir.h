#ifndef PIR_H_
#define PIR_H_

#include <openssl/evp.h>

#include <string>
#include <unordered_map>

#include "ssot.h"

namespace PIR {

template <typename D>
void DPF_PIR(Peer peer[2], FSS1Bit &fss, std::vector<D> array_23[2], const uint n, const uint log_n, const uint index_23[2], D &v_out_13, bool pseudo, bool count_band) {
    debug_print("[%u]DPF_PIR, index_23 = (%u, %u), log_n = %u\n", n, index_23[0], index_23[1], log_n);
    // fprintf(stderr, "[%u]DPF_PIR, index_23 = (%u, %u), log_n = %u\n", n, index_23[0], index_23[1], log_n);
    // only accept power of 2 n

    // std::chrono::high_resolution_clock::time_point t1, t2;
    // t1 = std::chrono::high_resolution_clock::now();
    // uint64_t time;

    const bool is_symmetric = array_23[0][0].IsSymmetric();

    BinaryData query_23[2];
    bool is_0 = false;
    if (pseudo) {
        uint data_length = divide_ceil(n, 8);
        fss.PseudoGen(peer, index_23[0] ^ index_23[1], data_length, is_symmetric, query_23, is_0);
        peer[0].WriteData(query_23[0], count_band);
        peer[1].ReadData(query_23[0]);
    } else {
        fss.Gen(index_23[0] ^ index_23[1], log_n, is_symmetric, query_23, is_0);

        peer[0].WriteData(query_23[0], count_band);
        peer[1].WriteData(query_23[1], count_band);

        peer[0].ReadData(query_23[1]);
        peer[1].ReadData(query_23[0]);
    }

    // t2 = std::chrono::high_resolution_clock::now();
    // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    // fprintf(stderr, "time1 = %llu\n", time);

    uint data_size = array_23[0][0].Size();
    D v_sum[2] = {D(data_size), D(data_size)};
    std::vector<bool> dpf_out(n);

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

    // t2 = std::chrono::high_resolution_clock::now();
    // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    // fprintf(stderr, "time2 = %llu\n", time);

    if (is_symmetric) {
        v_out_13 = v_sum[0] + v_sum[1];
    } else {
        v_out_13 = inv_gadget::Inv(peer, is_0, v_sum, count_band);
    }

    // t2 = std::chrono::high_resolution_clock::now();
    // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    // fprintf(stderr, "time3 = %llu\n", time);
}

template <typename K>
uint DPF_KEY_PIR(uint party, Peer peer[2], FSS1Bit &fss, std::vector<K> &key_array_13, const K key_23[2], EVP_MD_CTX *md_ctx, uchar *sha256_digest, bool count_band) {
    uint n = key_array_13.size();
    debug_print("[%u]DPF_KEY_PIR\n", n);

    // std::chrono::high_resolution_clock::time_point t1, t2;
    // t1 = std::chrono::high_resolution_clock::now();
    // uint64_t time;

    const uint digest_size = 4;
    const uint digest_size_log = digest_size * 8;
    const EVP_MD *sha256 = EVP_sha256();

    uint v_sum = 0;
    if (party == 2) {
        const uint P0 = 1;
        K key = key_23[0] + key_23[1];
        key.Print("key");
        std::vector<uchar> key_dump = key.Dump();
        // print_bytes(key_dump.data(), key_dump.size(), "key_dump");

        for (uint b = 0; b < 2; b++) {
            if (b == 1) {
                key_dump[0] ^= 1;
            }

            EVP_DigestInit_ex(md_ctx, sha256, NULL);
            EVP_DigestUpdate(md_ctx, key_dump.data(), key_dump.size());
            uint sha256_digest_size;
            EVP_DigestFinal_ex(md_ctx, sha256_digest, &sha256_digest_size);
            uint64_t digest_uint;
            memcpy(&digest_uint, sha256_digest, digest_size);

            // print_bytes(digest, digest_size, "digest");

            BinaryData query_23[2];
            bool is_0 = false;

            fss.Gen(digest_uint, digest_size_log, true, query_23, is_0);

            peer[0].WriteUInt(query_23[0].Size(), count_band);
            peer[1].WriteUInt(query_23[1].Size(), count_band);

            peer[0].WriteData(query_23[0], count_band);
            peer[1].WriteData(query_23[1], count_band);
        }
        v_sum = rand_uint(peer[P0].PRG());
    } else {  // party == 0 || party == 1
        // std::vector<bool> dpf_out[2];
        BinaryData query[2];
        for (uint b = 0; b < 2; b++) {
            uint query_size = peer[party].ReadUInt();
            query[b].Resize(query_size);
            peer[party].ReadData(query[b]);
        }
        // t2 = std::chrono::high_resolution_clock::now();
        // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        // fprintf(stderr, "time1 = %llu\n", time);

        std::vector<uchar> key_dump_array[key_array_13.size()];
        uint64_t key_array_digest[key_array_13.size()];
        std::unordered_map<uint64_t, uint> exists;
        for (uint i = 0; i < key_array_13.size(); i++) {
            K key = key_array_13[i] - key_23[1 - party];

            // t2 = std::chrono::high_resolution_clock::now();
            // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            // fprintf(stderr, "time1.1 = %llu\n", time);

            key_dump_array[i] = key.Dump();

            // t2 = std::chrono::high_resolution_clock::now();
            // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            // fprintf(stderr, "time1.2 = %llu\n", time);

            EVP_DigestInit_ex(md_ctx, sha256, NULL);
            EVP_DigestUpdate(md_ctx, key_dump_array[i].data(), key_dump_array[i].size());
            uint sha256_digest_size;
            EVP_DigestFinal_ex(md_ctx, sha256_digest, &sha256_digest_size);

            // t2 = std::chrono::high_resolution_clock::now();
            // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            // fprintf(stderr, "time1.3 = %llu\n", time);

            uint64_t digest_uint;
            memcpy(&digest_uint, sha256_digest, digest_size);
            key_array_digest[i] = digest_uint;

            // t2 = std::chrono::high_resolution_clock::now();
            // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            // fprintf(stderr, "time1.4 = %llu\n", time);

            if (exists.find(digest_uint) == exists.end()) {
                exists[digest_uint] = 1;
            } else {
                exists[digest_uint]++;
            }
        }
        // t2 = std::chrono::high_resolution_clock::now();
        // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        // fprintf(stderr, "time2 = %llu\n", time);

        for (uint i = 0; i < key_array_13.size(); i++) {
            uint64_t key_digest = key_array_digest[i];
            if (exists[key_digest] == 1) {
                // debug_print("dpf i = %u, exists = 1\n", i);
                // print_bytes((uchar *)&key_digest, digest_size, "key_digest");
                if (fss.Eval(query[0], key_digest)) {
                    // debug_print("dpf i = %u, fss.Eval = true\n", i);
                    v_sum ^= i;
                }
            } else {  // collision
                key_dump_array[i][0] ^= 1;
                EVP_DigestInit_ex(md_ctx, sha256, NULL);
                EVP_DigestUpdate(md_ctx, key_dump_array[i].data(), key_dump_array[i].size());
                uint sha256_digest_size;
                EVP_DigestFinal_ex(md_ctx, sha256_digest, &sha256_digest_size);

                uint64_t sha256_digest_uint64;
                memcpy((uchar *)&sha256_digest_uint64, sha256_digest, digest_size);
                if (fss.Eval(query[1], sha256_digest_uint64)) {
                    v_sum ^= i;
                }
                // collision can still happen...
            }
        }
        // t2 = std::chrono::high_resolution_clock::now();
        // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        // fprintf(stderr, "time3 = %llu\n", time);

        if (party == 0) {
            const uint P2 = 0;
            v_sum ^= rand_uint(peer[P2].PRG());
        }
    }
    // t2 = std::chrono::high_resolution_clock::now();
    // time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    // fprintf(stderr, "time4 = %llu\n", time);
    return v_sum % n;
}

template <typename D>
void SSOT_PIR(uint party, Peer peer[2], std::vector<D> &array_13, const uint index_23[2], D &v_out_13, bool count_band) {
    // TODO n may not be power of 2
    uint n = array_13.size();
    debug_print("[%lu]SSOT_PIR, n = %u, index_23 = (%u, %u)\n", array_13.size(), n, index_23[0], index_23[1]);
    uint data_size = array_13[0].Size();
    if (party == 2) {
        // const uint P1 = 0, P0 = 1;
        const uint P0 = 1;
        peer[P0].WriteData(array_13, count_band);
        SSOT::P2<D>(peer, n, data_size, count_band);
        v_out_13.Random(peer[P0].PRG());
    } else if (party == 0) {
        const uint P2 = 0, P1 = 1;

        std::vector<D> u(n, D(data_size));
        peer[P2].ReadData(u);
        for (uint i = 0; i < n; i++) {
            u[i] += array_13[i];
        }
        v_out_13 = SSOT::P0(peer, index_23[P1] ^ index_23[P2], u, count_band);

        D tmp(data_size);
        tmp.Random(peer[P2].PRG());
        v_out_13 -= tmp;
    } else {  // this->party_ == 1
        // const uint P0 = 0, P2 = 1;
        const uint P2 = 1;
        v_out_13 = SSOT::P1(peer, index_23[P2], array_13, count_band);
    }
}

template <typename D>
void PIR(Peer peer[2], FSS1Bit &fss, std::vector<D> array_23[2], const uint index_23[2], D &v_out_13, bool count_band) {
    uint n = pow2_ceil(array_23[0].size());
    uint log_n = log2(n);
    uint clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%lu]PIR, n = %u\n", array_23[0].size(), n);
    if (n == 1) {
        v_out_13 = array_23[0][0];
    } else {
        bool pseudo = (n <= PSEUDO_DPF_THRESHOLD);
        DPF_PIR(peer, fss, array_23, n, log_n, clean_index_23, v_out_13, pseudo, count_band);
    }
}

};  // namespace PIR

#endif /* PIR_H_ */