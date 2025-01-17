#ifndef SSOT_H_
#define SSOT_H_

namespace SSOT {
template <typename D>
void P2(Peer peer[2], const uint n, const uint data_size, Benchmark::Record* benchmark) {
#ifdef BENCHMARK_SSOT
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::SSOT.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    const uint P1 = 0, P0 = 1;
    debug_print("SSOT::P2, data_size = %u\n", data_size);

    D delta(data_size);
    delta.Random();

    uint alpha = rand_uint(peer[P0].PRG()) % n;
    uint beta = rand_uint(peer[P1].PRG()) % n;

    D x(data_size), y(data_size), xx(data_size), yy(data_size);
    for (uint i = 0; i < n; i++) {
        x.Random(peer[P0].PRG());
        y.Random(peer[P1].PRG());
        if (i == beta) {
            xx = x + delta;
        }
        if (i == alpha) {
            yy = y - delta;
        }
    }

    // Send x0, x1, y, alpha to P0
    peer[P0].WriteData(yy, benchmark);
    // Send y0, y1, x, beta to P1
    peer[P1].WriteData(xx, benchmark);
#ifdef BENCHMARK_SSOT
    if (benchmark != NULL) {
        Benchmark::SSOT.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
}

template <typename D>
D P0(Peer peer[2], const uint b0, std::vector<D>& u, Benchmark::Record* benchmark) {
#ifdef BENCHMARK_SSOT
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::SSOT.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    const uint P2 = 0, P1 = 1;
    // Receive x0, x1, y, alpha from P2

    uint n = u.size();
    uint data_size = u[0].Size();
    debug_print("SSOT::P0, b0 = %u, n = %u, data_size = %u\n", b0, n, data_size);

    D y(data_size);
    peer[P2].ReadData(y, benchmark);

    uint alpha = rand_uint(peer[P2].PRG()) % n;

    std::vector<D> x(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        x[i].Random(peer[P2].PRG());
    }

    // Send s to P1
    uint s = b0 ^ alpha;
    peer[P1].WriteUInt(s, benchmark);

    // Receive t from P1
    uint t = peer[P1].ReadUInt(benchmark);

    // Send u0' and u1' to P1
    std::vector<D> uu(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        uu[i] = u[b0 ^ i] + x[t ^ i];
    }
    // peer[P1].WriteData(uu, benchmark);

    // // Receive v0' and v1' from P1
    // std::vector<D> vv = peer[P1].ReadData(n, data_size);

    std::vector<D> vv(n, D(data_size));
    write_read_data(peer[P1], uu, peer[P1], vv, benchmark);
#ifdef BENCHMARK_SSOT
    if (benchmark != NULL) {
        Benchmark::SSOT.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
    return vv[b0] - y;
}

template <typename D>
D P1(Peer peer[2], const uint b1, std::vector<D>& v, Benchmark::Record* benchmark) {
#ifdef BENCHMARK_SSOT
    uint old_bandwidth;
    if (benchmark != NULL) {
        Benchmark::SSOT.Start();
        old_bandwidth = benchmark->bandwidth_;
    }
#endif
    const uint P0 = 0,
               P2 = 1;
    // print_bytes(v01[0], data_size, "v01", 0);
    // print_bytes(v01[1], data_size, "v01", 1);
    // Receive y0, y1, x, beta from P2

    uint n = v.size();
    uint data_size = v[0].Size();
    debug_print("SSOT::P1, b1 = %u, n = %u, data_size = %u\n", b1, n, data_size);

    D x(data_size);
    peer[P2].ReadData(x, benchmark);

    uint beta = rand_uint(peer[P2].PRG()) % n;

    std::vector<D> y(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        y[i].Random(peer[P2].PRG());
    }

    // Send t to P0
    uint t = b1 ^ beta;
    peer[P0].WriteUInt(t, benchmark);

    // Receive s from P0
    uint s = peer[P0].ReadUInt(benchmark);

    // Send v0' and v1' to P0
    std::vector<D> vv(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        vv[i] = v[b1 ^ i] + y[s ^ i];
    }
    // peer[P0].WriteData(vv, benchmark);

    // // Receive u0' and u1' from P0
    // std::vector<D> uu = peer[P0].ReadData(n, data_size);

    std::vector<D> uu(n, D(data_size));
    write_read_data(peer[P0], vv, peer[P0], uu, benchmark);
#ifdef BENCHMARK_SSOT
    if (benchmark != NULL) {
        Benchmark::SSOT.Stop(benchmark->bandwidth_ - old_bandwidth);
    }
#endif
    return uu[b1] - x;
}

};  // namespace SSOT

#endif /* SSOT_H_ */
