#ifndef INV_GADGET_H_
#define INV_GADGET_H_

#include <inttypes.h>

#include "peer.h"
#include "typedef.h"
#include "util.h"

namespace inv_gadget {

template <typename D>
void P0(Peer peer[2], const bool b, const uint data_size, const bool inv, bool count_band) {
    const uint P1 = inv ? 0 : 1;
    const uint P2 = inv ? 1 : 0;
    // debug_print("inv::P0 inv = %u\n", inv);

    std::vector<D> p(2, D(data_size));
    for (uint b = 0; b < 2; b++) {
        p[b].Random();
    }
    uchar s = rand_bool();
    uchar bb = b ^ s;

    peer[P1].Socket().Write(&s, 1, count_band);
    peer[P1].WriteData(p, count_band);

    peer[P2].Socket().Write(&bb, 1, count_band);
    peer[P2].WriteData(p[b], count_band);
}

template <typename D>
void P1(Peer peer[2], D m[2], const bool inv, bool count_band) {
    const uint P2 = inv ? 0 : 1;
    const uint P0 = inv ? 1 : 0;
    // debug_print("inv::P1 inv = %u\n", inv);

    uchar s;
    peer[P0].Socket().Read(&s, 1);

    uint data_size = m[0].Size();
    std::vector<D> p(2, D(data_size));
    peer[P0].ReadData(p);

    std::vector<D> mm(2);
    for (uint b = 0; b < 2; b++) {
        mm[b] = m[b ^ s] + p[b ^ s];
    }

    peer[P2].WriteData(mm, count_band);
}

template <typename D>
D P2(Peer peer[2], const uint data_size, const bool inv, bool count_band) {
    const uint P0 = inv ? 0 : 1;
    const uint P1 = inv ? 1 : 0;
    // debug_print("inv::P2 inv = %u\n", inv);

    uchar bb;
    peer[P0].Socket().Read(&bb, 1);
    D pb(data_size);
    peer[P0].ReadData(pb);
    std::vector<D> mm(2, D(data_size));
    peer[P1].ReadData(mm);

    return mm[bb] - pb;
}

template <typename D>
D Inv(Peer peer[2], const bool is_0, D v[2], bool count_band) {
    debug_print("Inv, is_0 = %u\n", is_0);
    uint data_size = v[0].Size();

    P0<D>(peer, is_0, data_size, false, count_band);
    D r(data_size);
    r.Random();
    D m[2] = {v[1] - r, -v[1] - r};
    P1<D>(peer, m, false, count_band);
    D mb = P2<D>(peer, data_size, false, count_band);
    D v_inv = r + mb;

    // inv
    P0<D>(peer, !is_0, data_size, true, count_band);
    r.Random();
    m[0] = v[0] - r;
    m[1] = -v[0] - r;
    P1(peer, m, true, count_band);
    mb = P2<D>(peer, data_size, true, count_band);
    v_inv += r + mb;
    return v_inv;
}

}  // namespace inv_gadget

#endif /* INV_GADGET_H_ */
