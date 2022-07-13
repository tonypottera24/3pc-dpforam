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

#ifdef BENCHMARK_PIR
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::PIR_GEN_DPF.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif

    BinaryData query_23[2];
    bool is_0 = false;
    if (pseudo) {
        uint data_length = divide_ceil(n, 8);
        is_0 = fss.PseudoGen(peer, index_23[0] ^ index_23[1], data_length, D::IsSymmetric(), query_23, benchmark);
    } else {
        is_0 = fss.Gen(peer, index_23[0] ^ index_23[1], log_n, D::IsSymmetric(), false, query_23, benchmark);
    }

#ifdef BENCHMARK_PIR
    if (benchmark != NULL) {
        Benchmark::PIR_GEN_DPF.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif

    uint data_size = array_23[0][0].Size();
    D v_sum[2] = {D(data_size), D(data_size)};
    std::vector<uchar> dpf_out(n);

    for (uint b = 0; b < 2; b++) {
#ifdef BENCHMARK_PIR
        uint old_bandwidth;
        if (benchmark != NULL) {
            Benchmark::PIR_EVAL_DPF.Start();
            old_bandwidth = benchmark->bandwidth_;
        }
#endif
        if (pseudo) {
            fss.PseudoEvalAll(query_23[b], n, dpf_out, benchmark);
        } else {
            fss.EvalAll(query_23[b], log_n, dpf_out, benchmark);
        }
#ifdef BENCHMARK_PIR
        if (benchmark != NULL) {
            Benchmark::PIR_EVAL_DPF.Stop(benchmark->bandwidth_ - old_bandwidth);

            old_bandwidth = benchmark->bandwidth_;
            Benchmark::PIR_ADD_DATA.Start();
        }
#endif
        for (uint i = 0; i < array_23[0].size(); i++) {
            // debug_print("[%u]DPF_PIR, i = %u, ii = %u, dpf_out = %u\n", n, i, i ^ index_23[b], (uint)dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                v_sum[b] += array_23[b][i];
            }
        }
#ifdef BENCHMARK_PIR
        if (benchmark != NULL) {
            Benchmark::PIR_ADD_DATA.Stop(benchmark->bandwidth_ - old_bandwidth);
        }
#endif
    }

    if (D::IsSymmetric()) {
        return v_sum[0] + v_sum[1];
    } else {
#ifdef BENCHMARK_PIR
        uint old_bandwidth;
        if (benchmark != NULL) {
            Benchmark::PIR_GROUP_PREPARE.Start();
            old_bandwidth = benchmark->bandwidth_;
        }
#endif
        D ans = inv_gadget::Inv(peer, is_0, v_sum, benchmark);
#ifdef BENCHMARK_PIR
        if (benchmark != NULL) {
            Benchmark::PIR_GROUP_PREPARE.Stop(benchmark->bandwidth_ - old_bandwidth);
        }
#endif
        return ans;
    }
}

template <typename K>
uint DPF_KEY_PIR(uint party, Peer peer[2], FSS1Bit &fss, std::vector<K> &key_array_13, const K key_23[2], Benchmark::Record *benchmark) {
    uint n = key_array_13.size();
    debug_print("[%u]DPF_KEY_PIR\n", n);

    const uint64_t digest_size = 2;
    // const uint64_t digest_size = 3;
    const uint64_t digest_size_log = digest_size * 8;
    const uint64_t digest_n = 1ULL << digest_size_log;
    // bool eval_all[2] = {n > KEY_VALUE_EVALALL_THRESHOLD, false};
    bool eval_all[2] = {false, false};
    std::vector<uint64_t> key_array_digest_(n);
    // uint key_size = key_array_13[0].Size();

    uint v_sum = 0;

    if (party == 2) {
        const uint P0 = 1;
        K key = key_23[0] + key_23[1];
        // key.Print("key");
#ifdef BENCHMARK_KEY_VALUE
        uint old_bandwidth = 0;
        if (benchmark != NULL) {
            Benchmark::KEY_VALUE_DPF_GEN.Start();
            old_bandwidth = benchmark->bandwidth_;
        }
#endif
        for (uint b = 0; b < 2; b++) {
            uint64_t digest_uint = key.hash(digest_n, b);
            // debug_print("digest_uint = %lu\n", digest_uint);
            BinaryData query_23[2];
            fss.Gen(peer, digest_uint, digest_size_log, true, true, query_23, benchmark);
        }
#ifdef BENCHMARK_KEY_VALUE
        if (benchmark != NULL) {
            Benchmark::KEY_VALUE_DPF_GEN.Stop(benchmark->bandwidth_ - old_bandwidth);
        }
#endif
        v_sum = rand_uint(peer[P0].PRG()) % n;
    } else {  // party == 0 || party == 1
#ifdef BENCHMARK_KEY_VALUE
        uint old_bandwidth = 0;
        if (benchmark != NULL) {
            Benchmark::KEY_VALUE_DPF_EVAL.Start();
            old_bandwidth = benchmark->bandwidth_;
        }
#endif
        BinaryData query[2];
        std::vector<uchar> dpf_out[2];
        for (uint b = 0; b < 2; b++) {
            uint query_size = peer[party].ReadUInt(benchmark);
            query[b].Resize(query_size);
            peer[party].ReadData(query[b], benchmark);
        }
        for (uint b = 0; b < 2; b++) {
            if (eval_all[b]) {
                dpf_out[b].resize(digest_n);
                fss.EvalAll(query[b], digest_size_log, dpf_out[b], benchmark);
            }
        }
#ifdef BENCHMARK_KEY_VALUE
        if (benchmark != NULL) {
            Benchmark::KEY_VALUE_DPF_EVAL.Stop(benchmark->bandwidth_ - old_bandwidth);
        }
#endif

        std::unordered_map<uint64_t, uint> exists;
        uint collision_ct = 0;

#ifdef BENCHMARK_KEY_VALUE
        if (benchmark != NULL) {
            Benchmark::KEY_VALUE_ADD_INDEX.Start();
            old_bandwidth = benchmark->bandwidth_;
        }
#endif

        for (uint i = 0; i < n; i++) {
            K key = key_array_13[i] - key_23[1 - party];
            uint64_t digest_uint = key.hash(digest_n, 0);
            if (exists.find(digest_uint) == exists.end()) {
                // debug_print("dpf i = %u, exists = 1\n", i);
                if (eval_all[0] ? dpf_out[0][digest_uint] : fss.Eval(query[0], digest_uint, benchmark)) {
                    v_sum ^= i;
                }
                exists[digest_uint] = 1;
            } else {  // collision
                // fprintf(stderr, "collision, i = %u\n", i);
                uint64_t digest_uint = key.hash(digest_n, 1);
                if (eval_all[1] ? dpf_out[1][digest_uint] : fss.Eval(query[1], digest_uint, benchmark)) {
                    v_sum ^= i;
                }
                exists[digest_uint]++;
                collision_ct++;
                // collision can still happen...
            }
        }
        // fprintf(stderr, "collision_ct = %u\n", collision_ct);

#ifdef BENCHMARK_KEY_VALUE
        if (benchmark != NULL) {
            Benchmark::KEY_VALUE_ADD_INDEX.Stop(benchmark->bandwidth_ - old_bandwidth);
        }
#endif

        if (party == 0) {
            const uint P2 = 0;
            v_sum ^= rand_uint(peer[P2].PRG()) % n;
        }
    }

    return v_sum % n;
}

// template <typename D>
// D SSOT_PIR(uint party, Peer peer[2], std::vector<D> &array_13, const uint index_23[2], Benchmark::Record *benchmark) {
//     // TODO n may not be power of 2
//     uint n = array_13.size();
//     debug_print("[%lu]SSOT_PIR, n = %u, index_23 = (%u, %u)\n", array_13.size(), n, index_23[0], index_23[1]);
//     uint data_size = array_13[0].Size();
//     D v_out_13(data_size);
//     if (party == 2) {
//         // const uint P1 = 0, P0 = 1;
//         const uint P0 = 1;
//         peer[P0].WriteDataVector(array_13, benchmark);
//         SSOT::P2<D>(peer, n, data_size, benchmark);
//         v_out_13.Random(peer[P0].PRG());
//     } else if (party == 0) {
//         const uint P2 = 0, P1 = 1;

//         std::vector<D> u(n, D(data_size));
//         peer[P2].ReadDataVector(u, benchmark);
//         for (uint i = 0; i < n; i++) {
//             u[i] += array_13[i];
//         }
//         v_out_13 = SSOT::P0(peer, index_23[P1] ^ index_23[P2], u, benchmark);

//         D tmp(data_size);
//         tmp.Random(peer[P2].PRG());
//         v_out_13 -= tmp;
//     } else {  // this->party_ == 1
//         // const uint P0 = 0, P2 = 1;
//         const uint P2 = 1;
//         v_out_13 = SSOT::P1(peer, index_23[P2], array_13, benchmark);
//     }
//     return v_out_13;
// }

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