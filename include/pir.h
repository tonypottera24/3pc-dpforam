#ifndef PIR_H_
#define PIR_H_

#include "dpf_oram.h"

template <typename D>
template <typename DD>
DD &DPFORAM<D>::PIR(std::vector<DD> array_23[2], const uint64_t index_23[2], bool count_band) {
    uint64_t n = uint64_pow2_ceil(array_23[0].size());
    uint64_t log_n = uint64_log2(n);
    uint64_t clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%llu]PIR, n = %llu\n", this->Size(), n);
    if (n == 1) {
        return *new DD(array_23[0][0]);
    } else if (n <= this->pseudo_dpf_threshold_) {
        return this->PSEUDO_DPF_PIR(array_23, n, log_n, clean_index_23, count_band);
    } else {
        return this->DPF_PIR(array_23, n, log_n, clean_index_23, count_band);
    }
}

template <typename D>
template <typename DD>
DD &DPFORAM<D>::DPF_PIR(std::vector<DD> array_23[2], const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], bool count_band) {
    debug_print("[%llu]DPF_PIR, n = %llu, log_n = %llu\n", this->Size(), n, log_n);
    // only accept power of 2 n
    const bool is_symmetric = array_23[0][0].IsSymmetric();

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, is_symmetric, query_23);

    this->conn_[0]->Write(query_23[0], query_size, count_band);
    this->conn_[1]->Write(query_23[1], query_size, count_band);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    uint data_size = array_23[0][0].Size();
    DD *v_out_13 = new DD(data_size, true);
    for (uint b = 0; b < 2; b++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[b], log_n, dpf_out);
        for (uint64_t i = 0; i < array_23[b].size(); i++) {
            // debug_print("[%llu]DPF_PIR, i = %llu, ii = %llu, dpf_out = %u\n", this->Size(), i, i ^ index_23[b], dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                if (b == 0) {
                    *v_out_13 += array_23[b][i];
                } else {
                    *v_out_13 -= array_23[b][i];
                }
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] query_23[b];
    }
    return *v_out_13;
}

template <typename D>
template <typename DD>
DD &DPFORAM<D>::PSEUDO_DPF_PIR(std::vector<DD> array_23[2], const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], bool count_band) {
    debug_print("[%llu]PSEUDO_DPF_PIR, n = %llu, index_23 = (%llu, %llu)\n", this->Size(), n, index_23[0], index_23[1]);
    // only accept power of 2 n
    const bool is_symmetric = array_23[0][0].IsSymmetric();

    uint64_t data_length = uint64_ceil_divide(n, 8ULL);
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

    uint data_size = array_23[0][0].Size();
    DD *v_out_13 = new DD(data_size, true);
    for (uint b = 0; b < 2; b++) {
        bool dpf_out_evaluated[n];
        this->fss_.PseudoEvalAll(query[b], n, dpf_out_evaluated);
        for (uint64_t i = 0; i < array_23[b].size(); i++) {
            // debug_print("[%llu]PSEUDO_DPF_PIR, i = %llu, ii = %llu, dpf_out_evaluated = %u\n", this->Size(), i, i ^ index_23[b], dpf_out_evaluated[i ^ index_23[b]]);
            if (dpf_out_evaluated[i ^ index_23[b]]) {
                if (b == 0) {
                    *v_out_13 += array_23[b][i];
                } else {
                    *v_out_13 -= array_23[b][i];
                }
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] query[b];
    }
    return *v_out_13;
}

template <typename D>
template <typename DD>
DD &DPFORAM<D>::SSOT_PIR(std::vector<DD> &array_13, const uint64_t index_23[2], bool count_band) {
    // TODO n may not be power of 2
    uint64_t n = array_13.size();
    debug_print("[%llu]SSOT_PIR, n = %llu, index_23 = (%llu, %llu)\n", this->Size(), n, index_23[0], index_23[1]);
    uint data_size = array_13[0].Size();
    DD *v_out_13;
    if (this->party_ == 2) {
        // const uint P1 = 0, P0 = 1;
        const uint P0 = 1;

        this->conn_[P0]->WriteData(array_13, count_band);

        this->SSOT_P2(n, data_size, count_band);

        v_out_13 = new D(data_size, true);
        v_out_13->Random(this->prgs_[P0]);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;

        std::vector<DD> u = this->conn_[P2]->template ReadData<DD>(n, data_size);
        for (uint64_t i = 0; i < n; i++) {
            u[i] += array_13[i];
        }
        v_out_13 = this->SSOT_P0(index_23[P1] ^ index_23[P2], u, count_band);

        DD tmp = DD(data_size);
        tmp.Random(this->prgs_[P2]);
        *v_out_13 -= tmp;
    } else {  // this->party_ == 1
        // const uint P0 = 0, P2 = 1;
        const uint P2 = 1;
        v_out_13 = this->SSOT_P1(index_23[P2], array_13, count_band);
    }
    return *v_out_13;
}

#endif /* PIR_H_ */