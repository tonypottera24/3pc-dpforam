#ifndef PIW_H_
#define PIW_H_

namespace PIW {

template <typename D>
void DPF_PIW(Peer peer[2], FSS1Bit &fss, std::vector<D> &array_13, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], D v_delta_23[2], bool count_band) {
    debug_print("[%lu]DPF_PIW, index_23 = (%llu, %llu), n = %lu, log_n = %llu, new n = %llu\n", array_13.size(), index_23[0], index_23[1], array_13.size(), log_n, n);
    v_delta_23[0].Print("v_delta_23[0]");
    v_delta_23[1].Print("v_delta_23[1]");
    const bool is_symmetric = array_13[0].IsSymmetric();

    std::vector<BinaryData> query_23;
    bool is_0;
    std::tie(query_23, is_0) = fss.Gen(index_23[0] ^ index_23[1], log_n, is_symmetric);

    peer[0].WriteData(query_23[0], count_band);
    peer[1].WriteData(query_23[1], count_band);

    query_23[1] = peer[0].template ReadData<BinaryData>(query_23[0].Size());
    query_23[0] = peer[1].template ReadData<BinaryData>(query_23[1].Size());

    if (!is_symmetric) {
        D v_delta_13 = inv_gadget::Inv(peer, is_0, v_delta_23, count_band);
        v_delta_23 = ShareTwoThird<D>(peer, v_delta_13, count_band);
    }

    for (uint b = 0; b < 2; b++) {
        uchar *dpf_out = fss.EvalAll(query_23[b], log_n);
        for (uint64_t i = 0; i < array_13.size(); i++) {
            // debug_print("[%llu]DPF_PIW, i = %llu, ii = %llu, dpf_out = %u\n", this->Size(), i, i ^ index_23[b], dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                array_13[i] += v_delta_23[b];
            }
        }
    }
}

template <typename D>
void PSEUDO_DPF_PIW(Peer peer[2], FSS1Bit &fss, std::vector<D> &array_13, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], D v_delta_23[2], bool count_band) {
    debug_print("[%lu]PSEUDO_DPF_PIW, index_23 = (%llu, %llu), n = %llu\n", array_13.size(), index_23[0], index_23[1], n);
    uint64_t data_length = uint64_ceil_divide(n, 8ULL);
    const bool is_symmetric = array_13[0].IsSymmetric();

    std::vector<BinaryData> query_23;
    bool is_0;
    std::tie(query_23, is_0) = fss.PseudoGen(peer, index_23[0] ^ index_23[1], data_length, is_symmetric);
    peer[0].WriteData(query_23[0], count_band);
    query_23[0] = peer[1].template ReadData<BinaryData>(query_23[0].Size());

    if (!is_symmetric) {
        D v_delta_13 = inv_gadget::Inv(peer, is_0, v_delta_23, count_band);
        v_delta_23 = ShareTwoThird<D>(peer, v_delta_13, count_band);
    }

    for (uint b = 0; b < 2; b++) {
        bool *dpf_out_evaluated = fss.PseudoEvalAll(query_23[b], n);
        for (uint64_t i = 0; i < array_13.size(); i++) {
            if (dpf_out_evaluated[i ^ index_23[b]]) {
                array_13[i] += v_delta_23[b];
            }
        }
    }
}

template <typename D>
void PIW(Peer peer[2], FSS1Bit &fss, std::vector<D> &array_13, const uint64_t index_23[2], D v_delta_23[2], uint64_t pseudo_dpf_threshold, bool count_band) {
    uint64_t n = uint64_pow2_ceil(array_13.size());
    uint64_t log_n = uint64_log2(n);
    uint64_t clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%lu]PIW, index_23 = (%llu, %llu), n = %llu\n", array_13.size(), index_23[0], index_23[1], n);
    if (n == 1) {
        array_13[0] += v_delta_23[0];
    } else if (n <= pseudo_dpf_threshold) {
        PSEUDO_DPF_PIW(peer, fss, array_13, n, log_n, clean_index_23, v_delta_23, count_band);
    } else {
        DPF_PIW(peer, fss, array_13, n, log_n, clean_index_23, v_delta_23, count_band);
    }
}

};  // namespace PIW

#endif /* PIW_H_ */