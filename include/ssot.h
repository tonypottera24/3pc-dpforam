#ifndef SSOT_H_
#define SSOT_H_

namespace SSOT {
template <typename D>
void P2(Peer peer[2], const uint n, const uint data_size, bool count_band) {
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
    peer[P0].WriteData(yy, count_band);
    // Send y0, y1, x, beta to P1
    peer[P1].WriteData(xx, count_band);
}

template <typename D>
D P0(Peer peer[2], const uint b0, std::vector<D>& u, bool count_band) {
    const uint P2 = 0, P1 = 1;
    // Receive x0, x1, y, alpha from P2

    uint n = u.size();
    uint data_size = u[0].Size();
    debug_print("SSOT::P0, b0 = %u, n = %u, data_size = %u\n", b0, n, data_size);

    D y(data_size);
    peer[P2].ReadData(y);

    uint alpha = rand_uint(peer[P2].PRG()) % n;

    std::vector<D> x(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        x[i].Random(peer[P2].PRG());
    }

    // Send s to P1
    uint s = b0 ^ alpha;
    peer[P1].WriteUInt(s, count_band);

    // Receive t from P1
    uint t = peer[P1].ReadUInt();

    // Send u0' and u1' to P1
    std::vector<D> uu(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        uu[i] = u[b0 ^ i] + x[t ^ i];
    }
    // peer[P1].WriteData(uu, count_band);

    // // Receive v0' and v1' from P1
    // std::vector<D> vv = peer[P1].ReadData(n, data_size);

    std::vector<D> vv(n, D(data_size));
    write_read_data(peer[P1], uu, peer[P1], vv, count_band);
    return vv[b0] - y;
}

template <typename D>
D P1(Peer peer[2], const uint b1, std::vector<D>& v, bool count_band) {
    const uint P0 = 0, P2 = 1;
    // print_bytes(v01[0], data_size, "v01", 0);
    // print_bytes(v01[1], data_size, "v01", 1);
    // Receive y0, y1, x, beta from P2

    uint n = v.size();
    uint data_size = v[0].Size();
    debug_print("SSOT::P1, b1 = %u, n = %u, data_size = %u\n", b1, n, data_size);

    D x(data_size);
    peer[P2].ReadData(x);

    uint beta = rand_uint(peer[P2].PRG()) % n;

    std::vector<D> y(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        y[i].Random(peer[P2].PRG());
    }

    // Send t to P0
    uint t = b1 ^ beta;
    peer[P0].WriteUInt(t, count_band);

    // Receive s from P0
    uint s = peer[P0].ReadUInt();

    // Send v0' and v1' to P0
    std::vector<D> vv(n, D(data_size));
    for (uint i = 0; i < n; i++) {
        vv[i] = v[b1 ^ i] + y[s ^ i];
    }
    // peer[P0].WriteData(vv, count_band);

    // // Receive u0' and u1' from P0
    // std::vector<D> uu = peer[P0].ReadData(n, data_size);

    std::vector<D> uu(n, D(data_size));
    write_read_data(peer[P0], vv, peer[P0], uu, count_band);
    return uu[b1] - x;
}

};  // namespace SSOT

#endif /* SSOT_H_ */
