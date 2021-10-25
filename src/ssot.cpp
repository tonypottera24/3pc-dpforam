#include "ssot.h"

SSOT::SSOT(const uint party, const DataType data_type, Connection *cons[2],
           CryptoPP::AutoSeededRandomPool *rnd,
           CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs) : Protocol(party, cons, rnd, prgs), data_type_(data_type) {
}

void SSOT::P2(const uint64_t n, const uint64_t data_size, bool count_band) {
    const uint P1 = 0, P0 = 1;
    // fprintf(stderr, "SSOT::P2, data_size = %u\n", data_size);

    Data delta = Data(this->data_type_, data_size);
    delta.Random(this->rnd_);

    uint64_t alpha = rand_uint64(&this->prgs_[P0]) % n;
    uint64_t beta = rand_uint64(&this->prgs_[P1]) % n;

    Data x = Data(this->data_type_, data_size);
    Data y = Data(this->data_type_, data_size);
    Data xx = Data(this->data_type_, data_size);
    Data yy = Data(this->data_type_, data_size);
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

Data *SSOT::P0(const uint64_t b0, std::vector<Data> &u, bool count_band) {
    const uint P2 = 0, P1 = 1;
    // fprintf(stderr, "SSOT::P0, b0 = %u, data_size = %u\n", b0, data_size);
    // Receive x0, x1, y, alpha from P2

    uint64_t n = u.size();
    uint data_size = u[0].Size();
    Data y = this->conn_[P2]->ReadData(this->data_type_, data_size);

    uint64_t alpha = rand_uint64(&this->prgs_[P2]) % n;

    Data *x = new Data[n];
    for (uint64_t i = 0; i < n; i++) {
        x[i] = Data(this->data_type_, data_size);
        x[i].Random(&this->prgs_[P2]);
    }

    // Send s to P1
    uint64_t s = b0 ^ alpha;
    this->conn_[P1]->WriteLong(s, count_band);

    // Receive t from P1
    uint64_t t = this->conn_[P1]->ReadLong();

    // Send u0' and u1' to P1
    std::vector<Data> uu;
    for (uint64_t i = 0; i < n; i++) {
        uu.emplace_back(u[b0 ^ i] + x[t ^ i]);
    }
    this->conn_[P1]->WriteData(uu, count_band);

    // Receive v0' and v1' from P1
    std::vector<Data> vv = this->conn_[P1]->ReadData(this->data_type_, n, data_size);
    return new Data(vv[b0] - y);
}

Data *SSOT::P1(const uint64_t b1, std::vector<Data> &v, bool count_band) {
    const uint P0 = 0, P2 = 1;
    // fprintf(stderr, "SSOT::P1, b1 = %u, data_size = %u\n", b1, data_size);
    // print_bytes(v01[0], data_size, "v01", 0);
    // print_bytes(v01[1], data_size, "v01", 1);
    // Receive y0, y1, x, beta from P2

    uint64_t n = v.size();
    uint data_size = v[0].Size();
    Data x = this->conn_[P2]->ReadData(this->data_type_, data_size);

    uint64_t beta = rand_uint64(&this->prgs_[P2]) % n;

    Data *y = new Data[n];
    for (uint64_t i = 0; i < n; i++) {
        y[i] = Data(this->data_type_, data_size);
        y[i].Random(&this->prgs_[P2]);
    }

    // Send t to P0
    uint64_t t = b1 ^ beta;
    this->conn_[P0]->WriteLong(t, count_band);

    // Receive s from P0
    uint64_t s = this->conn_[P0]->ReadLong();

    // Send v0' and v1' to P0
    std::vector<Data> vv;
    for (uint64_t i = 0; i < n; i++) {
        vv.emplace_back(v[b1 ^ i] + y[s ^ i]);
    }
    this->conn_[P0]->WriteData(vv, count_band);

    // Receive u0' and u1' from P0
    std::vector<Data> uu = this->conn_[P0]->ReadData(this->data_type_, n, data_size);
    return new Data(uu[b1] - x);
}

void SSOT::Test(uint iterations) {
    // for (uint iteration = 0; iteration < iterations; iteration++) {
    //     uint data_size = 50;
    //     uint b0 = this->rnd_->GenerateBit();
    //     uint b1 = this->rnd_->GenerateBit();
    //     uchar *u01[2] = {new uchar[data_size], new uchar[data_size]};
    //     uchar *v01[2] = {new uchar[data_size], new uchar[data_size]};
    //     for (uint b = 0; b < 2; b++) {
    //         this->rnd_->GenerateBlock(u01[b], data_size);
    //         this->rnd_->GenerateBlock(v01[b], data_size);
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

    //     for (uint b = 0; b < 2; b++) {
    //         delete[] u01[b];
    //         delete[] v01[b];
    //     }
    // }
}