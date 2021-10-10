#include "ssot.h"

SSOT::SSOT(const uint party, Connection *cons[2],
           CryptoPP::AutoSeededRandomPool *rnd,
           CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs) : Protocol(party, cons, rnd, prgs) {
}

void SSOT::P2(const uint64_t n, const uint64_t data_size, bool count_band) {
    const uint P1 = 0, P0 = 1;
    // fprintf(stderr, "SSOT::P2, data_size = %u\n", data_size);
    uchar *x[n], *y[n];
    for (uint64_t i = 0; i < n; i++) {
        x[i] = new uchar[data_size];
        y[i] = new uchar[data_size];
        this->prgs_[P0].GenerateBlock(x[i], data_size);
        this->prgs_[P1].GenerateBlock(y[i], data_size);
    }

    uchar delta[data_size];
    this->rnd_->GenerateBlock(delta, data_size);

    uint64_t alpha = rand_uint64(&this->prgs_[P0]) % n;
    uint64_t beta = rand_uint64(&this->prgs_[P1]) % n;

    uchar xx[data_size];
    xor_bytes(x[beta], delta, data_size, xx);
    uchar yy[data_size];
    xor_bytes(y[alpha], delta, data_size, yy);

    // Send x0, x1, y, alpha to P0
    this->conn_[P0]->Write(yy, data_size, count_band);
    // Send y0, y1, x, beta to P1
    this->conn_[P1]->Write(xx, data_size, count_band);
}

void SSOT::P0(const uint64_t b0, uchar **u, const uint64_t n, const uint64_t data_size, uchar *p0, bool count_band) {
    const uint P2 = 0, P1 = 1;
    // fprintf(stderr, "SSOT::P0, b0 = %u, data_size = %u\n", b0, data_size);
    // Receive x0, x1, y, alpha from P2
    uchar *x[n];
    for (uint64_t i = 0; i < n; i++) {
        x[i] = new uchar[data_size];
        this->prgs_[P2].GenerateBlock(x[i], data_size);
    }

    uchar y[data_size];
    this->conn_[P2]->Read(y, data_size);

    uint64_t alpha = rand_uint64(&this->prgs_[P2]) % n;

    // Send s to P1
    uint64_t s = b0 ^ alpha;
    this->conn_[P1]->WriteLong(s, count_band);

    // Receive t from P1
    uint64_t t = this->conn_[P1]->ReadLong();

    // Send u0' and u1' to P1
    uchar uu[data_size * n];
    for (uint64_t i = 0; i < n; i++) {
        xor_bytes(u[b0 ^ i], x[t ^ i], data_size, &uu[i * data_size]);
    }
    this->conn_[P1]->Write(uu, data_size * n, count_band);

    // Receive v0' and v1' from P1
    uchar vv[data_size * n];
    this->conn_[P1]->Read(vv, data_size * n);
    xor_bytes(&vv[b0 * data_size], y, data_size, p0);
}

void SSOT::P1(const uint64_t b1, uchar **v, const uint64_t n, const uint64_t data_size, uchar *p1, bool count_band) {
    const uint P0 = 0, P2 = 1;
    // fprintf(stderr, "SSOT::P1, b1 = %u, data_size = %u\n", b1, data_size);
    // print_bytes(v01[0], data_size, "v01", 0);
    // print_bytes(v01[1], data_size, "v01", 1);
    // Receive y0, y1, x, beta from P2
    uchar *y[n];
    for (uint64_t i = 0; i < n; i++) {
        y[i] = new uchar[data_size];
        this->prgs_[P2].GenerateBlock(y[i], data_size);
    }

    uchar x[data_size];
    this->conn_[P2]->Read(x, data_size);

    uint64_t beta = rand_uint64(&this->prgs_[P2]) % n;

    // Send t to P0
    uint64_t t = b1 ^ beta;
    this->conn_[P0]->WriteLong(t, count_band);

    // Receive s from P0
    uint64_t s = this->conn_[P0]->ReadLong();

    // Send v0' and v1' to P0
    uchar vv[data_size * n];
    for (uint64_t i = 0; i < n; i++) {
        xor_bytes(v[b1 ^ i], y[s ^ i], data_size, &vv[i * data_size]);
    }
    this->conn_[P0]->Write(vv, data_size * n, count_band);

    // Receive u0' and u1' from P0
    uchar uu[data_size * n];
    this->conn_[P0]->Read(uu, data_size * n);
    xor_bytes(&uu[b1 * data_size], x, data_size, p1);
}

void SSOT::Test(uint iterations) {
    // for (uint iteration = 0; iteration < iterations; iteration++) {
    //     uint data_size = 50;
    //     uint b0 = this->rnd_->GenerateBit();
    //     uint b1 = this->rnd_->GenerateBit();
    //     uchar *u01[2] = {new uchar[data_size], new uchar[data_size]};
    //     uchar *v01[2] = {new uchar[data_size], new uchar[data_size]};
    //     for (uint i = 0; i < 2; i++) {
    //         this->rnd_->GenerateBlock(u01[i], data_size);
    //         this->rnd_->GenerateBlock(v01[i], data_size);
    //     }
    //     uchar p0[data_size];
    //     uchar p1[data_size];
    //     if (this->party_ == 2) {
    //         P2(data_size);
    //     } else if (this->party_ == 0) {
    //         P0(b0, u01, data_size, p0);
    //         this->conn_[0]->WriteInt(b0);
    //         this->conn_[0]->Write(p0, data_size);
    //         this->conn_[0]->Write(u01[0], data_size);
    //         this->conn_[0]->Write(u01[1], data_size);
    //     } else if (this->party_ == 1) {
    //         P1(b1, v01, data_size, p1);
    //         b0 = this->conn_[1]->ReadInt();
    //         this->conn_[1]->Read(p0, data_size);
    //         this->conn_[1]->Read(u01[0], data_size);
    //         this->conn_[1]->Read(u01[1], data_size);
    //         uint b = b0 ^ b1;
    //         uchar output[data_size];
    //         uchar expected[data_size];
    //         xor_bytes(u01[b], v01[b], data_size, expected);
    //         xor_bytes(p0, p1, data_size, output);
    //         if (memcmp(output, expected, data_size) == 0) {
    //             fprintf(stderr, "SSOT passed: %u\n", iteration);
    //         } else {
    //             fprintf(stderr, "SSOT failed: %u\n", iteration);
    //         }
    //     } else {
    //         fprintf(stderr, "Incorrect party: %d\n", this->party_);
    //     }

    //     for (uint i = 0; i < 2; i++) {
    //         delete[] u01[i];
    //         delete[] v01[i];
    //     }
    // }
}