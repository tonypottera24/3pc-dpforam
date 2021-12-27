#ifndef PIR_H_
#define PIR_H_

#include "ssot.h"

namespace PIR {

template <typename D>
D DPF_PIR(Peer peer[2], FSS1Bit &fss, std::vector<D> array_23[2], const uint n, const uint log_n, const uint index_23[2], bool pseudo, bool count_band) {
    debug_print("[%u]DPF_PIR, n = %u, log_n = %u\n", n, n, log_n);
    // only accept power of 2 n
    const bool is_symmetric = array_23[0][0].IsSymmetric();

    std::vector<BinaryData> query_23;
    bool is_0 = false;
    if (pseudo) {
        uint data_length = divide_ceil(n, 8);
        std::tie(query_23, is_0) = fss.PseudoGen(peer, index_23[0] ^ index_23[1], data_length, is_symmetric);
        peer[0].WriteData(query_23[0], count_band);
        query_23[0] = peer[1].template ReadData<BinaryData>(query_23[0].Size());
    } else {
        uint index = index_23[0] ^ index_23[1];
        std::tie(query_23, is_0) = fss.Gen((uchar *)&index, log_n, is_symmetric);

        peer[0].WriteData(query_23[0], count_band);
        peer[1].WriteData(query_23[1], count_band);

        query_23[1] = peer[0].template ReadData<BinaryData>(query_23[0].Size());
        query_23[0] = peer[1].template ReadData<BinaryData>(query_23[1].Size());
    }

    uint data_size = array_23[0][0].Size();
    D v_sum[2] = {D(data_size, true), D(data_size, true)};
    for (uint b = 0; b < 2; b++) {
        bool *dpf_out;
        if (pseudo) {
            dpf_out = fss.PseudoEvalAll(query_23[b], n);
        } else {
            dpf_out = fss.EvalAll(query_23[b], log_n);
        }
        for (uint i = 0; i < array_23[0].size(); i++) {
            // debug_print("[%u]DPF_PIR, i = %u, ii = %u, dpf_out = %u\n", this->Size(), i, i ^ index_23[b], dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                v_sum[b] += array_23[b][i];
            }
        }
    }

    if (is_symmetric) {
        return v_sum[0] + v_sum[1];
    } else {
        return inv_gadget::Inv(peer, is_0, v_sum, count_band);
    }
}

template <typename K>
uint DPF_KEY_PIR(uint party, Peer peer[2], FSS1Bit &fss, std::vector<K> key_array_13, const K key_23[2], uint index_n, bool count_band) {
    debug_print("[%lu]DPF_KEY_PIR\n", key_array_13.size());
    // do not accept pseudo mode

    uint v_sum = 0;
    if (party == 2) {
        const uint P0 = 1;
        bool is_0 = false;
        std::vector<BinaryData> query_23;
        K key = key_23[0] + key_23[1];
        uint log_n = key.Size() * 8;
        std::tie(query_23, is_0) = fss.Gen(key.Dump(), log_n, true);

        peer[0].WriteUInt(query_23[0].Size(), count_band);
        peer[1].WriteUInt(query_23[1].Size(), count_band);

        peer[0].WriteData(query_23[0], count_band);
        peer[1].WriteData(query_23[1], count_band);

        v_sum = rand_uint(peer[P0].PRG());
    } else {  // party == 0 || party == 1
        uint query_size = peer[party].ReadUInt();
        BinaryData query = peer[party].template ReadData<BinaryData>(query_size);
        for (uint i = 0; i < key_array_13.size(); i++) {
            K key = key_array_13[i] - key_23[1 - party];
            if (fss.Eval(query, key.Dump())) {
                v_sum ^= i;
            }
        }

        if (party == 0) {
            const uint P2 = 0;
            v_sum ^= rand_uint(peer[P2].PRG());
        }
    }
    return v_sum % index_n;
}

template <typename D>
D SSOT_PIR(uint party, Peer peer[2], std::vector<D> &array_13, const uint index_23[2], bool count_band) {
    // TODO n may not be power of 2
    uint n = array_13.size();
    debug_print("[%lu]SSOT_PIR, n = %u, index_23 = (%u, %u)\n", array_13.size(), n, index_23[0], index_23[1]);
    uint data_size = array_13[0].Size();
    D v_out_13;
    if (party == 2) {
        // const uint P1 = 0, P0 = 1;
        const uint P0 = 1;
        peer[P0].WriteData(array_13, count_band);
        SSOT::P2<D>(peer, n, data_size, count_band);
        v_out_13.Random(peer[P0].PRG(), data_size);
    } else if (party == 0) {
        const uint P2 = 0, P1 = 1;

        std::vector<D> u = peer[P2].template ReadData<D>(n, data_size);
        for (uint i = 0; i < n; i++) {
            u[i] += array_13[i];
        }
        v_out_13 = SSOT::P0(peer, index_23[P1] ^ index_23[P2], u, count_band);

        D tmp;
        tmp.Random(peer[P2].PRG(), data_size);
        v_out_13 -= tmp;
    } else {  // this->party_ == 1
        // const uint P0 = 0, P2 = 1;
        const uint P2 = 1;
        v_out_13 = SSOT::P1(peer, index_23[P2], array_13, count_band);
    }
    return v_out_13;
}

template <typename D>
D PIR(Peer peer[2], FSS1Bit &fss, std::vector<D> array_23[2], const uint index_23[2], uint pseudo_dpf_threshold, bool count_band) {
    uint n = pow2_ceil(array_23[0].size());
    uint log_n = log2(n);
    uint clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%lu]PIR, n = %u\n", array_23[0].size(), n);
    if (n == 1) {
        return array_23[0][0];
    } else {
        bool pseudo = (n <= pseudo_dpf_threshold);
        return DPF_PIR(peer, fss, array_23, n, log_n, clean_index_23, pseudo, count_band);
    }
}

};  // namespace PIR

#endif /* PIR_H_ */