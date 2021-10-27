#ifndef PIW_H_
#define PIW_H_

#include "dpf_oram.h"

template <typename D>
template <typename DD>
void DPFORAM<D>::PIW(std::vector<DD> &array_13, const uint64_t index_23[2], DD v_delta_23[2], bool count_band) {
    uint64_t n = uint64_pow2_ceil(array_13.size());
    uint64_t log_n = uint64_log2(n);
    uint64_t clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%llu]PIW, index_23 = (%llu, %llu), n = %llu\n", this->Size(), index_23[0], index_23[1], n);
    if (n == 1) {
        array_13[0] += v_delta_23[0];
    } else if (n <= this->pseudo_dpf_threshold_) {
        this->PSEUDO_DPF_PIW(array_13, n, log_n, clean_index_23, v_delta_23, count_band);
    } else {
        this->DPF_PIW(array_13, n, log_n, clean_index_23, v_delta_23, count_band);
    }
}

template <typename D>
template <typename DD>
void DPFORAM<D>::DPF_PIW(std::vector<DD> &array_13, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], DD v_delta_23[2], bool count_band) {
    debug_print("[%llu]DPF_PIW, index_23 = (%llu, %llu), n = %lu, log_n = %llu, new n = %llu\n", this->Size(), index_23[0], index_23[1], array_13.size(), log_n, n);
    v_delta_23[0].Print("v_delta_23[0]");
    v_delta_23[1].Print("v_delta_23[1]");
    const bool is_symmetric = array_13[0].IsSymmetric();

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, is_symmetric, query_23);

    this->conn_[0]->Write(query_23[0], query_size, count_band);
    this->conn_[1]->Write(query_23[1], query_size, count_band);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    for (uint b = 0; b < 2; b++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[b], log_n, dpf_out);
        for (uint64_t i = 0; i < array_13.size(); i++) {
            // debug_print("[%llu]DPF_PIW, i = %llu, ii = %llu, dpf_out = %u\n", this->Size(), i, i ^ index_23[b], dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                if (b == 0) {
                    array_13[i] += v_delta_23[b];
                } else {
                    array_13[i] -= v_delta_23[b];
                }
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] query_23[b];
    }
}

template <typename D>
template <typename DD>
void DPFORAM<D>::PSEUDO_DPF_PIW(std::vector<DD> &array_13, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], DD v_delta_23[2], bool count_band) {
    debug_print("[%llu]PSEUDO_DPF_PIW, index_23 = (%llu, %llu), n = %llu\n", this->Size(), index_23[0], index_23[1], n);
    uint64_t data_length = uint64_ceil_divide(n, 8ULL);
    const bool is_symmetric = array_13[0].IsSymmetric();

    uchar *query[2];
    for (uint b = 0; b < 2; b++) {
        query[b] = new uchar[data_length];
    }

    uchar flipped = this->fss_.PseudoGen(this->prgs_[1], index_23[0] ^ index_23[1], data_length, is_symmetric, query[0]);
    this->conn_[0]->Write(query[0], data_length, count_band);
    this->conn_[1]->Read(query[0], data_length);
    this->prgs_[0].GenerateBlock(query[1], data_length);

    if (!is_symmetric) {
        this->conn_[1]->Write(&flipped, 1, count_band);
        this->conn_[0]->Read(&flipped, 1);
        if (flipped) {
            for (uint64_t i = 0; i < data_length; i++) {
                query[1][i] = ~query[1][i];
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        bool dpf_out_evaluated[n];
        this->fss_.PseudoEvalAll(query[b], n, dpf_out_evaluated);
        for (uint64_t i = 0; i < array_13.size(); i++) {
            if (dpf_out_evaluated[i ^ index_23[b]]) {
                if (b == 0) {
                    array_13[i] += v_delta_23[b];
                } else {
                    array_13[i] -= v_delta_23[b];
                }
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] query[b];
    }
}

#endif /* PIW_H_ */