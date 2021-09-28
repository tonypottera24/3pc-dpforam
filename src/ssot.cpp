#include "ssot.h"

SSOT::SSOT(const uint party, Connection *cons[2],
           CryptoPP::AutoSeededRandomPool *rnd,
           CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs) : Protocol(party, cons, rnd, prgs) {
}

void SSOT::P2(const uint data_size, bool count_band) {
    const uint P1 = 0, P0 = 1;
    fprintf(stderr, "SSOT::P2, data_size = %u\n", data_size);
    uchar x01[2][data_size];
    this->prgs_[P0].GenerateBlock(x01[0], data_size);
    this->prgs_[P0].GenerateBlock(x01[1], data_size);

    uchar y01[2][data_size];
    this->prgs_[P1].GenerateBlock(y01[0], data_size);
    this->prgs_[P1].GenerateBlock(y01[1], data_size);

    uchar delta[data_size];
    this->rnd_->GenerateBlock(delta, data_size);

    uint alpha = this->prgs_[P0].GenerateBit();
    uint beta = this->prgs_[P1].GenerateBit();

    uchar x[data_size];
    xor_bytes(x01[beta], delta, data_size, x);
    uchar y[data_size];
    xor_bytes(y01[alpha], delta, data_size, y);

    // Send x0, x1, y, alpha to P0
    this->conn_[P0]->Write(y, data_size, count_band);
    // Send y0, y1, x, beta to P1
    this->conn_[P1]->Write(x, data_size, count_band);
}

void SSOT::P0(const uint b0, const uchar *u01[2], const uint data_size, uchar *p0, bool count_band) {
    const uint P2 = 0, P1 = 1;
    fprintf(stderr, "SSOT::P0, b0 = %u, data_size = %u\n", b0, data_size);
    print_bytes(u01[0], data_size, "u01", 0);
    print_bytes(u01[1], data_size, "u01", 1);
    // Receive x0, x1, y, alpha from P2
    uchar x01[2][data_size];
    this->prgs_[P2].GenerateBlock(x01[0], data_size);
    this->prgs_[P2].GenerateBlock(x01[1], data_size);

    uchar y[data_size];
    this->conn_[P2]->Read(y, data_size);

    uint alpha = this->prgs_[P2].GenerateBit();

    // Send s to P1
    uint s = b0 ^ alpha;
    this->conn_[P1]->WriteInt(s, count_band);

    // Receive t from P1
    uint t = this->conn_[P1]->ReadInt();

    // Send u0' and u1' to P1
    uchar u01_p[2][data_size];
    xor_bytes(u01[b0], x01[t], data_size, u01_p[0]);
    xor_bytes(u01[1 - b0], x01[1 - t], data_size, u01_p[1]);
    this->conn_[P1]->Write(u01_p[0], data_size, count_band);
    this->conn_[P1]->Write(u01_p[1], data_size, count_band);

    // Receive v0' and v1' from P1
    uchar v01_p[2][data_size];
    this->conn_[P1]->Read(v01_p[0], data_size);
    this->conn_[P1]->Read(v01_p[1], data_size);

    // Compute p_0
    xor_bytes(v01_p[b0], y, data_size, p0);
}

void SSOT::P1(const uint b1, const uchar *v01[2], const uint data_size, uchar *p1, bool count_band) {
    const uint P0 = 0, P2 = 1;
    fprintf(stderr, "SSOT::P1, b1 = %u, data_size = %u\n", b1, data_size);
    print_bytes(v01[0], data_size, "v01", 0);
    print_bytes(v01[1], data_size, "v01", 1);
    // Receive y0, y1, x, beta from P2
    uchar y01[2][data_size];
    this->prgs_[P2].GenerateBlock(y01[0], data_size);
    this->prgs_[P2].GenerateBlock(y01[1], data_size);

    uchar x[data_size];
    this->conn_[P2]->Read(x, data_size);

    uint beta = this->prgs_[P2].GenerateBit();

    // Send t to P0
    uint t = b1 ^ beta;
    this->conn_[P0]->WriteInt(t, count_band);

    // Receive s from P0
    uint s = this->conn_[P0]->ReadInt();

    // Send v0' and v1' to P0
    uchar v01_p[2][data_size];
    xor_bytes(v01[b1], y01[s], data_size, v01_p[0]);
    xor_bytes(v01[1 - b1], y01[1 - s], data_size, v01_p[1]);
    this->conn_[P0]->Write(v01_p[0], data_size, count_band);
    this->conn_[P0]->Write(v01_p[1], data_size, count_band);

    // Receive u0' and u1' from P0
    uchar u01_p[2][data_size];
    this->conn_[P0]->Read(u01_p[0], data_size);
    this->conn_[P0]->Read(u01_p[1], data_size);

    // Compute p_1
    xor_bytes(u01_p[b1], x, data_size, p1);
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