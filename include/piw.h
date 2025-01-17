#ifndef PIW_H_
#define PIW_H_

namespace PIW {

template <typename D>
std::vector<D> FindDeltaData(uint party, Peer peer[2], bool is_0, D &v_delta_13, Benchmark::Record *benchmark) {
    debug_print("FindDeltaData, is_0 = %u\n", is_0);
    const uint data_size = v_delta_13.Size();
    std::vector<D> v_out_33(2, D(data_size));
    if (D::IsSymmetric()) {
        ShareTwoThird(peer, v_delta_13, v_out_33.data(), benchmark);
    } else {
        v_delta_13.Print("v_delta_13");

        D v_delta_33[2] = {D(data_size), D(data_size)};
        v_delta_33[0].Random();
        v_delta_33[1] = v_delta_13 - v_delta_33[0];
        v_delta_33[0].Print("v_delta_33[0]");
        v_delta_33[1].Print("v_delta_33[1]");

        for (uint d = 0; d < 2; d++) {
            for (uint offset = 0; offset < 3; offset++) {
                uint party_index = (party + offset) % 3;
                if (party_index == 2) {
                    uchar is_02[2];
                    rand_bytes(&is_02[0], 1);
                    is_02[0] &= 1;
                    if (d == 0) {
                        is_02[1] = is_02[0] ^ is_0;
                    } else {
                        is_02[1] = is_02[0] ^ !is_0;
                    }
                    debug_print("FindDeltaData, P2, d = %u, is_0 = %u, is_02 = (%u, %u)\n", d, is_0, is_02[0], is_02[1]);
                    for (uint b = 0; b < 2; b++) {
                        peer[b].Socket().Write(&is_02[b], 1, benchmark);
                    }
                    SSOT::P2<D>(peer, 2, data_size, benchmark);
                } else if (party_index == 0) {  // P0
                    uint P2 = 0, P1 = 1;
                    uchar is_02;
                    peer[P2].Socket().Read(&is_02, 1, benchmark);
                    debug_print("FindDeltaData, P0, is_02 = %u\n", is_02);
                    v_delta_33[1].Print("v_delta_33[1]");
                    (-v_delta_33[1]).Print("-v_delta_33[1]");
                    std::vector<D> u = {v_delta_33[1], -v_delta_33[1]};
                    D w0 = SSOT::P0(peer, is_02, u, benchmark);
                    w0.Print("w0");
                    if (d == 0) {
                        peer[P1].WriteData(w0, benchmark);
                    } else {
                        D w1(data_size);
                        peer[P1].ReadData(w1, benchmark);
                        v_out_33[1] = w0 + w1;
                        v_out_33[1].Print("v_out_33[1]");
                    }
                } else {  // P1
                    uint P0 = 0, P2 = 1;
                    uchar is_02;
                    peer[P2].Socket().Read(&is_02, 1, benchmark);
                    debug_print("FindDeltaData, P1, is_02 = %u\n", is_02);
                    v_delta_33[0].Print("v_delta_33[0]");
                    (-v_delta_33[0]).Print("-v_delta_33[0]");
                    std::vector<D> v = {v_delta_33[0], -v_delta_33[0]};
                    D w1 = SSOT::P1(peer, is_02, v, benchmark);
                    w1.Print("w1");
                    if (d == 0) {
                        D w0(data_size);
                        peer[P0].ReadData(w0, benchmark);
                        v_out_33[0] = w0 + w1;
                        v_out_33[0].Print("v_out_33[0]");
                    } else {
                        peer[P0].WriteData(w1, benchmark);
                    }
                }
            }
        }
    }
    return v_out_33;
}

template <typename D>
void DPF_PIW(uint party, Peer peer[2], FSS1Bit &fss, std::vector<D> &array_13, const uint n, const uint log_n, const uint index_23[2], D &v_delta_13, bool pseudo, Benchmark::Record *benchmark) {
    debug_print("[%lu]DPF_PIW, index_23 = (%u, %u), n = %lu, log_n = %u, new n = %u\n", array_13.size(), index_23[0], index_23[1], array_13.size(), log_n, n);
    v_delta_13.Print("v_delta_13");

#ifdef BENCHMARK_PIW
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::PIW_GEN_DPF.Start();
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
    debug_print("is_0 = %u\n", is_0);

#ifdef BENCHMARK_PIW
    if (benchmark != NULL) {
        Benchmark::PIW_GEN_DPF.Stop(benchmark->bandwidth_ - old_bandwidth);

        old_bandwidth = benchmark->bandwidth_;
        Benchmark::PIW_GROUP_PREPARE.Start();
    }
#endif
    std::vector<D> v_delta_33 = FindDeltaData(party, peer, is_0, v_delta_13, benchmark);
#ifdef BENCHMARK_PIW
    if (benchmark != NULL) {
        Benchmark::PIW_GROUP_PREPARE.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif

    std::vector<uchar> dpf_out(n);
    for (uint b = 0; b < 2; b++) {
#ifdef BENCHMARK_PIW
        uint old_bandwidth;
        if (benchmark != NULL) {
            Benchmark::PIW_EVAL_DPF.Start();
            old_bandwidth = benchmark->bandwidth_;
        }
#endif
        if (pseudo) {
            fss.PseudoEvalAll(query_23[b], n, dpf_out, benchmark);
        } else {
            fss.EvalAll(query_23[b], log_n, dpf_out, benchmark);
        }
#ifdef BENCHMARK_PIW
        if (benchmark != NULL) {
            Benchmark::PIW_EVAL_DPF.Stop(benchmark->bandwidth_ - old_bandwidth);

            old_bandwidth = benchmark->bandwidth_;
            Benchmark::PIW_ADD_DATA.Start();
        }
#endif
        for (uint i = 0; i < array_13.size(); i++) {
            debug_print("[%lu]DPF_PIW, i = %u, ii = %u, dpf_out = %u\n", array_13.size(), i, i ^ index_23[b], dpf_out[i ^ index_23[b]]);
            if (dpf_out[i ^ index_23[b]]) {
                array_13[i] += v_delta_33[b];
            }
        }
#ifdef BENCHMARK_PIW
        if (benchmark != NULL) {
            Benchmark::PIW_ADD_DATA.Stop(benchmark->bandwidth_ - old_bandwidth);
        }
#endif
    }
}

template <typename D>
void PIW(uint party, Peer peer[2], FSS1Bit &fss, std::vector<D> &array_13, const uint index_23[2], D &v_delta_13, Benchmark::Record *benchmark) {
    uint n = pow2_ceil(array_13.size());
    uint log_n = log2(n);
    uint clean_index_23[2] = {index_23[0] % n, index_23[1] % n};
    debug_print("[%lu]PIW, index_23 = (%u, %u), n = %u\n", array_13.size(), index_23[0], index_23[1], n);
    if (n == 1) {
        array_13[0] += v_delta_13;
    } else {
        bool pseudo = (n <= PSEUDO_DPF_THRESHOLD);
        DPF_PIW(party, peer, fss, array_13, n, log_n, clean_index_23, v_delta_13, pseudo, benchmark);
    }
}

};  // namespace PIW

#endif /* PIW_H_ */