#ifndef SSOT_H_
#define SSOT_H_

#include <iostream>

#include "protocol.h"
#include "util.h"

template <typename D>
class SSOT : public Protocol {
private:
    const DataType data_type_;

public:
    SSOT(const uint party, const DataType data_type, Connection* cons[2],
         CryptoPP::AutoSeededRandomPool* rnd,
         CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption* prgs) : Protocol(party, cons, rnd, prgs), data_type_(data_type) {
    }

    void P2(const uint64_t n, const uint64_t data_size, bool count_band = true) {
        const uint P1 = 0, P0 = 1;
        // fprintf(stderr, "SSOT::P2, data_size = %u\n", data_size);

        D delta = D(this->data_type_, data_size);
        delta.Random(this->rnd_);

        uint64_t alpha = rand_uint64(&this->prgs_[P0]) % n;
        uint64_t beta = rand_uint64(&this->prgs_[P1]) % n;

        D x = D(this->data_type_, data_size);
        D y = D(this->data_type_, data_size);
        D xx = D(this->data_type_, data_size);
        D yy = D(this->data_type_, data_size);
        for (uint64_t i = 0; i < n; i++) {
            x.Random(&this->prgs_[P0]);
            y.Random(&this->prgs_[P1]);
            if (i == beta) {
                xx = x + delta;
            }
            if (i == alpha) {
                yy = y - delta;
            }
        }

        // Send x0, x1, y, alpha to P0
        this->conn_[P0]->WriteData(yy, count_band);
        // Send y0, y1, x, beta to P1
        this->conn_[P1]->WriteData(xx, count_band);
    }

    D* P0(const uint64_t b0, std::vector<D>& u, bool count_band = true) {
        const uint P2 = 0, P1 = 1;
        // fprintf(stderr, "SSOT::P0, b0 = %u, data_size = %u\n", b0, data_size);
        // Receive x0, x1, y, alpha from P2

        uint64_t n = u.size();
        uint data_size = u[0].Size();
        D y = this->conn_[P2]->ReadData(this->data_type_, data_size);

        uint64_t alpha = rand_uint64(&this->prgs_[P2]) % n;

        D* x = new D[n];
        for (uint64_t i = 0; i < n; i++) {
            x[i] = D(this->data_type_, data_size);
            x[i].Random(&this->prgs_[P2]);
        }

        // Send s to P1
        uint64_t s = b0 ^ alpha;
        this->conn_[P1]->WriteLong(s, count_band);

        // Receive t from P1
        uint64_t t = this->conn_[P1]->ReadLong();

        // Send u0' and u1' to P1
        std::vector<D> uu;
        for (uint64_t i = 0; i < n; i++) {
            uu.emplace_back(u[b0 ^ i] + x[t ^ i]);
        }
        this->conn_[P1]->WriteData(uu, count_band);

        // Receive v0' and v1' from P1
        std::vector<D> vv = this->conn_[P1]->ReadData(this->data_type_, n, data_size);
        return new D(vv[b0] - y);
    }

    D* P1(const uint64_t b1, std::vector<D>& v, bool count_band = true) {
        const uint P0 = 0, P2 = 1;
        // fprintf(stderr, "SSOT::P1, b1 = %u, data_size = %u\n", b1, data_size);
        // print_bytes(v01[0], data_size, "v01", 0);
        // print_bytes(v01[1], data_size, "v01", 1);
        // Receive y0, y1, x, beta from P2

        uint64_t n = v.size();
        uint data_size = v[0].Size();
        D x = this->conn_[P2]->ReadData(this->data_type_, data_size);

        uint64_t beta = rand_uint64(&this->prgs_[P2]) % n;

        D* y = new D[n];
        for (uint64_t i = 0; i < n; i++) {
            y[i] = D(this->data_type_, data_size);
            y[i].Random(&this->prgs_[P2]);
        }

        // Send t to P0
        uint64_t t = b1 ^ beta;
        this->conn_[P0]->WriteLong(t, count_band);

        // Receive s from P0
        uint64_t s = this->conn_[P0]->ReadLong();

        // Send v0' and v1' to P0
        std::vector<D> vv;
        for (uint64_t i = 0; i < n; i++) {
            vv.emplace_back(v[b1 ^ i] + y[s ^ i]);
        }
        this->conn_[P0]->WriteData(vv, count_band);

        // Receive u0' and u1' from P0
        std::vector<D> uu = this->conn_[P0]->ReadData(this->data_type_, n, data_size);
        return new D(uu[b1] - x);
    }

    void Test(uint iter) {}
};

#endif /* SSOT_H_ */