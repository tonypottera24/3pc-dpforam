#include "dpf_oram.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const uint party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
                 uint64_t n, uint data_size, uint tau) : Protocol(party, connections, rnd, prgs) {
    fprintf(stderr, "DPFORAM n = %llu, data_size = %u, tau = %u\n", n, data_size, tau);
    this->n_ = n;
    this->data_size_ = data_size;
    this->tau_ = tau;

    // first level position map, can be contained by two blocks.

    for (uint i = 0; i < 2; i++) {
        this->InitArray(this->read_array_23_[i]);
        this->InitArray(this->cache_23_[i]);
    }
    this->InitArray(this->write_array_13_);
    this->cache_ctr_ = 0;

    uint64_t pos_data_per_block = 1 << this->tau_;
    uint64_t pos_n = (this->n_ + pos_data_per_block - 1) / pos_data_per_block;
    uint64_t log_n = ceil(log2(this->n_));
    uint64_t pos_data_size = ((log_n + 7) / 8 + 1) * pos_data_per_block;
    if (this->n_ > 1) {
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, pos_n, pos_data_size, tau);
    }
}

DPFORAM::~DPFORAM() {
    if (this->position_map_ != NULL) {
        delete this->position_map_;
    }
    for (uint i = 0; i < 2; i++) {
        this->DeleteArray(this->read_array_23_[i]);
        this->DeleteArray(this->cache_23_[i]);
    }
    this->DeleteArray(this->write_array_13_);
}

void DPFORAM::Reset() {
    for (uint i = 0; i < 2; i++) {
        this->DeleteArray(this->read_array_23_[i]);
        this->InitArray(this->read_array_23_[i]);
        this->DeleteArray(this->cache_23_[i]);
        this->InitArray(this->cache_23_[i]);
    }
    this->DeleteArray(this->write_array_13_);
    this->InitArray(this->write_array_13_);
    this->cache_ctr_ = 0;
    if (this->position_map_ != NULL) {
        this->position_map_->Reset();
    }
}

void DPFORAM::InitArray(uchar **&array) {
    array = new uchar *[this->n_];
    for (uint i = 0; i < this->n_; i++) {
        array[i] = new uchar[this->data_size_]();  // init to zero
    }
}

void DPFORAM::DeleteArray(uchar **array) {
    if (array == NULL) return;
    for (uint i = 0; i < this->n_; i++) {
        delete[] array[i];
    }
    delete[] array;
}

void DPFORAM::PIR(uchar **array_23[2], const uint64_t n, const uint data_size, const uint64_t index_23[2], uchar *v_out) {
    fprintf(stderr, "[%llu]PIR\n", this->n_);
    uint log_n = ceil(log2(n));

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    // uchar dpf_out[n];
    // this->fss_.EvalAll(query_23[0], log_n, dpf_out);
    // if (dpf_out[index_23[0] ^ index_23[1]] == 0) {
    //     // Make sure query[0] has one more 1 than query[1].
    //     fprintf(stderr, "PIR GG0.6\n");
    //     std::swap(query_23[0], query_23[1]);
    // }

    this->conn_[0]->Write(query_23[0], query_size);
    this->conn_[1]->Write(query_23[1], query_size);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    uchar *v[2];
    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[i], log_n, dpf_out);
        v[i] = new uchar[data_size]();
        for (uint64_t j = 0; j < n; j++) {
            // fprintf(stderr, "PIR i = %u, j = %llu, index_23[i] = %llu, jj = %llu, n = %llu\n", i, j, index_23[i], j ^ index_23[i], n);
            if (dpf_out[j ^ index_23[i]]) {
                xor_bytes(v[i], array_23[i][j], data_size, v[i]);
            }
        }
    }
    xor_bytes(v[0], v[1], data_size, v_out);

    for (uint i = 0; i < 2; i++) {
        delete[] query_23[i];
        delete[] v[i];
    }
    fprintf(stderr, "PIR finished\n");
}

void DPFORAM::PIW(uchar **array, const uint64_t n, const uint data_size, const uint64_t index_23[2], const uchar *v_delta) {
    fprintf(stderr, "[%llu]PIW, n = %llu\n", this->n_, n);
    uint log_n = ceil(log2(n));

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    // uchar dpf_out[n];
    // this->fss_.EvalAll(query_23[0], log_n, dpf_out);
    // if (dpf_out[index_23[0] ^ index_23[1]] == 0) {
    //     // Make sure query[0] has one more 1 than query[1].
    //     std::swap(query_23[0], query_23[1]);
    // }

    this->conn_[0]->Write(query_23[0], query_size);
    this->conn_[1]->Write(query_23[1], query_size);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[i], log_n, dpf_out);
        for (uint64_t j = 0; j < n; j++) {
            // fprintf(stderr, "PIW i = %u, j = %llu, index_23[i] = %llu, jj = %llu, n = %llu\n", i, j, index_23[i], j ^ index_23[i], n);
            if (dpf_out[j ^ index_23[i]]) {
                xor_bytes(array[j], v_delta, data_size, array[j]);
            }
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] query_23[i];
    }
}

void DPFORAM::GetLatestData(const uchar *v_read_13,
                            const uchar *v_cache_13, const uchar *v_meta_13,
                            uchar *v_out_13) {
    fprintf(stderr, "[%llu]GetLatestData, v_meta_13 = %d\n", this->n_, v_meta_13[0]);
    uchar *v_read_23[2];
    uchar *v_cache_23[2];
    uchar *v_meta_23[2];
    for (uint i = 0; i < 2; i++) {
        v_read_23[i] = new uchar[this->data_size_];
        v_cache_23[i] = new uchar[this->data_size_];
        v_meta_23[i] = new uchar[1];
    }
    this->ShareTwoThird(v_read_13, this->data_size_, v_read_23);
    this->ShareTwoThird(v_cache_13, this->data_size_, v_cache_23);
    this->ShareTwoThird(v_meta_13, 1, v_meta_23, true);

    SSOT *ssot = new SSOT(this->party_, this->conn_, this->rnd_, this->prgs_);
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;
        ssot->P2(this->data_size_);
        memset(v_out_13, 0, this->data_size_);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;
        uchar v_read_12[this->data_size_];
        xor_bytes(v_read_23[P1], v_read_23[P2], this->data_size_, v_read_12);

        uchar v_cache_12[this->data_size_];
        xor_bytes(v_cache_23[P1], v_cache_23[P2], this->data_size_, v_cache_12);

        uchar v_meta_12[1];
        xor_bytes(v_meta_23[P1], v_meta_23[P2], 1, v_meta_12);

        const uchar *u01[2] = {v_read_12, v_cache_12};
        const uint b0 = v_meta_12[0] & 1;
        ssot->P0(b0, u01, this->data_size_, v_out_13);
    } else if (this->party_ == 1) {
        const uint P0 = 0, P2 = 1;
        const uchar *v01[2] = {v_read_23[P0], v_cache_23[P2]};
        const uint b1 = v_meta_23[P2][0] & 1;
        ssot->P1(b1, v01, this->data_size_, v_out_13);
    }

    delete ssot;
    for (uint i = 0; i < 2; i++) {
        delete[] v_read_23[i];
        delete[] v_cache_23[i];
        delete[] v_meta_23[i];
    }
}

void DPFORAM::ShareTwoThird(const uchar *v_in_13, const uint data_size, uchar *v_out_23[2], bool count_band) {
    fprintf(stderr, "[%llu]ShareTwoThird\n", this->n_);
    this->rnd_->GenerateBlock(v_out_23[0], data_size);
    xor_bytes(v_in_13, v_out_23[0], data_size, v_out_23[1]);

    this->conn_[0]->Write(v_out_23[0], data_size, count_band);
    this->conn_[1]->Write(v_out_23[1], data_size, count_band);

    uchar tmp[data_size];
    this->conn_[0]->Read(tmp, data_size);
    xor_bytes(v_out_23[0], tmp, data_size, v_out_23[0]);
    this->conn_[1]->Read(tmp, data_size);
    xor_bytes(v_out_23[1], tmp, data_size, v_out_23[1]);
}

void DPFORAM::ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], uchar v_meta_13[1]) {
    fprintf(stderr, "[%llu]ReadPositionMap\n", this->n_);

    uint64_t data_size = ((uint64_t)ceil(log2(this->n_)) + 7) / 8 + 1;
    uint64_t data_per_block = 1U << this->tau_;

    uint64_t block_index_23[2];
    uint64_t data_index_23[2];
    uchar *old_block_23[2];
    for (uint i = 0; i < 2; i++) {
        block_index_23[i] = index_23[i] / data_per_block;
        data_index_23[i] = index_23[i] % data_per_block;
        old_block_23[i] = new uchar[this->position_map_->data_size_];
    }

    // read block from array
    this->position_map_->Read(block_index_23, old_block_23);

    // read data from block
    uchar **old_data_array_23[2];
    for (uint i = 0; i < 2; i++) {
        old_data_array_23[i] = new uchar *[data_per_block];
        for (uint j = 0; j < data_per_block; j++) {
            old_data_array_23[i][j] = new uchar[data_size];
        }
        bytes_to_bytes_array(old_block_23[i], this->position_map_->data_size_, data_per_block, old_data_array_23[i]);
    }
    uchar old_data_13[data_size];
    PIR(old_data_array_23, data_per_block, data_size, data_index_23, old_data_13);

    // parse data
    memcpy(v_meta_13, old_data_13, 1);
    uchar *data_23[2];
    for (uint i = 0; i < 2; i++) {
        data_23[i] = new uchar[data_size - 1];
    }
    this->ShareTwoThird(&old_data_13[1], data_size - 1, data_23);
    for (uint i = 0; i < 2; i++) {
        cache_index_23[i] = bytes_to_uint64(data_23[i], data_size - 1);
    }

    // write data to block
    uchar *data_array_13[data_per_block];
    for (uint i = 0; i < data_per_block; i++) {
        data_array_13[i] = new uchar[data_size];
        xor_bytes(old_data_array_23[0][i], old_data_array_23[1][i], data_size, data_array_13[i]);
    }
    uchar new_data_13[data_size];
    new_data_13[0] = 1;
    uint64_to_bytes(this->cache_ctr_, &new_data_13[1], data_size - 1);
    uchar delta_data_13[data_size];
    xor_bytes(old_data_13, new_data_13, data_size, delta_data_13);
    PIW(data_array_13, data_per_block, data_size, data_index_23, delta_data_13);

    // write block back to array
    uchar new_block_13[this->position_map_->data_size_];
    bytes_array_to_bytes(data_array_13, data_per_block, data_size, new_block_13);

    uchar old_block_13[this->position_map_->data_size_];
    xor_bytes(old_block_23[0], old_block_23[1], this->position_map_->data_size_, old_block_13);

    this->position_map_->Write(block_index_23, old_block_13, new_block_13);

    // cleanup
    for (uint i = 0; i < 2; i++) {
        delete[] old_block_23[i];
        for (uint j = 0; j < data_per_block; j++) {
            delete[] old_data_array_23[i][j];
        }
        delete[] old_data_array_23[i];
        delete[] data_23[i];
    }
    for (uint i = 0; i < data_per_block; i++) {
        delete[] data_array_13[i];
    }
}

void DPFORAM::Read(const uint64_t index_23[2], uchar *v_out_23[2]) {
    fprintf(stderr, "[%llu]Read\n", this->n_);

    // read v_r from the read array.
    uchar v_read_13[this->data_size_];
    PIR(this->read_array_23_, this->n_, this->data_size_, index_23, v_read_13);
    print_bytes(v_read_13, this->data_size_, "v_read_13");

    // read the position map.
    // if there's no position map, n == 1, the cache will always be empty
    // since the cache will be flushed immediately after write operation.
    uint64_t cache_index_23[2] = {};
    uchar v_meta_13[1] = {};
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, v_meta_13);
    }

    // read the cache.
    uchar v_cache_13[this->data_size_];
    PIR(this->cache_23_, this->cache_ctr_, this->data_size_, cache_index_23, v_cache_13);
    print_bytes(v_cache_13, this->data_size_, "v_cache_13");

    // Use SSOT to select v_r and v_cache by is_used
    uchar v_out_13[this->data_size_];
    GetLatestData(v_read_13, v_cache_13, v_meta_13, v_out_13);
    print_bytes(v_out_13, this->data_size_, "v_out_13");
    this->ShareTwoThird(v_out_13, this->data_size_, v_out_23);
}

void DPFORAM::Write(const uint64_t index_23[2], const uchar *old_data_13, const uchar *new_data_13) {
    fprintf(stderr, "[%llu]Write\n", this->n_);
    uchar delta_data_13[this->data_size_];
    xor_bytes(old_data_13, new_data_13, this->data_size_, delta_data_13);

    PIW(this->write_array_13_, this->n_, this->data_size_, index_23, delta_data_13);
    this->AppendCache(new_data_13);
}

void DPFORAM::AppendCache(const uchar *v_new_13) {
    fprintf(stderr, "[%llu]AppendCache\n", this->n_);
    uchar *v_new_23[2];
    for (uint i = 0; i < 2; i++) {
        v_new_23[i] = new uchar[this->data_size_];
    }
    this->ShareTwoThird(v_new_13, this->data_size_, v_new_23);
    for (uint i = 0; i < 2; i++) {
        memcpy(this->cache_23_[i][this->cache_ctr_], v_new_23[i], this->data_size_);
    }
    this->cache_ctr_++;

    if (this->cache_ctr_ == this->n_) {
        this->Flush();
        if (this->position_map_ != NULL) {
            this->position_map_->Reset();
        }
    }
    for (uint i = 0; i < 2; i++) {
        delete[] v_new_23[i];
    }
}

void DPFORAM::Flush() {
    fprintf(stderr, "[%llu]Flush\n", this->n_);
    uchar *read_array_23_i[2];
    for (uint64_t i = 0; i < this->n_; i++) {
        for (uint j = 0; j < 2; j++) {
            read_array_23_i[j] = this->read_array_23_[j][i];
        }
        this->ShareTwoThird(this->write_array_13_[i], this->data_size_, read_array_23_i);
    }
    this->cache_ctr_ = 0;
    for (uint i = 0; i < 2; i++) {
        for (uint64_t j = 0; j < this->n_; j++) {
            memset(this->cache_23_[i][j], 0, this->data_size_);
        }
    }
}

void DPFORAM::PrintMetadata() {
    fprintf(stderr, "===================\n");
    fprintf(stderr, "party_: %u\n", this->party_);
    fprintf(stderr, "tau_: %u\n", this->tau_);
    fprintf(stderr, "n_: %llu\n", this->n_);
    fprintf(stderr, "data_size_: %u\n", this->data_size_);
    fprintf(stderr, "cache_ctr_: %llu\n", this->cache_ctr_);
    fprintf(stderr, "position_map_: %d\n", (this->position_map_ != NULL));
    fprintf(stderr, "===================\n");
}

void DPFORAM::Test(uint iterations) {
    fprintf(stderr, "Test, iterations = %u \n", iterations);
    uint64_t party_time = 0;
    uint64_t time;

    PrintMetadata();

    for (uint iteration = 0; iteration < iterations; iteration++) {
        fprintf(stderr, "Test, iteration = %u\n", iteration);
        // randomly pick a index
        uint64_t log_n = ceil(log2(this->n_));

        uint64_t rand_range = (1 << (log_n - 1));
        fprintf(stderr, "Test, log_n = %llu, rand_range = %llu\n", log_n, rand_range);
        uint64_t index_23[2];
        index_23[0] = rand_uint64() % rand_range;
        this->conn_[0]->WriteLong(index_23[0], false);
        index_23[1] = this->conn_[1]->ReadLong();
        fprintf(stderr, "Test, index_23 = (%llu, %llu)\n", index_23[0], index_23[1]);

        // read old data for write
        uchar *old_data_23[2];
        for (uint i = 0; i < 2; i++) {
            old_data_23[i] = new uchar[this->data_size_];
        }
        time = timestamp();
        this->Read(index_23, old_data_23);
        party_time += timestamp() - time;
        print_bytes(old_data_23[0], this->data_size_, "old_data_23[0]");
        print_bytes(old_data_23[1], this->data_size_, "old_data_23[1]");

        // write randomly generated data
        uchar old_data_13[this->data_size_];
        xor_bytes(old_data_23[0], old_data_23[1], this->data_size_, old_data_13);

        uchar new_data_13[this->data_size_];
        this->rnd_->GenerateBlock(new_data_13, this->data_size_);

        print_bytes(new_data_13, this->data_size_, "new_data_13");
        time = timestamp();
        this->Write(index_23, old_data_13, new_data_13);
        party_time += timestamp() - time;

        // read data for validation.
        uchar *verify_data_23[2];
        for (uint i = 0; i < 2; i++) {
            verify_data_23[i] = new uchar[this->data_size_];
        }

        this->Read(index_23, verify_data_23);
        print_bytes(verify_data_23[0], this->data_size_, "verify_data_23[0]");
        print_bytes(verify_data_23[1], this->data_size_, "verify_data_23[1]");

        uchar *verify_data_33[3];
        for (uint i = 0; i < 3; i++) {
            verify_data_33[i] = new uchar[this->data_size_];
        }
        xor_bytes(verify_data_23[0], verify_data_23[1], this->data_size_, verify_data_33[2]);

        this->conn_[0]->Write(verify_data_33[2], this->data_size_, false);
        this->conn_[1]->Write(verify_data_33[2], this->data_size_, false);
        this->conn_[0]->Read(verify_data_33[0], this->data_size_);
        this->conn_[1]->Read(verify_data_33[1], this->data_size_);
        uchar verify_data_13[this->data_size_];
        xor_bytes(verify_data_33[0], verify_data_33[1], verify_data_33[2], this->data_size_, verify_data_13);
        if (memcmp(verify_data_13, new_data_13, this->data_size_) == 0) {
            fprintf(stderr, "%u, Pass\n", iteration);
        } else {
            fprintf(stderr, "%u, Fail !!!\n", iteration);
            exit(1);
        }

        for (uint i = 0; i < 2; i++) {
            delete[] old_data_23[i];
            delete[] verify_data_23[i];
        }
        for (uint i = 0; i < 3; i++) {
            delete[] verify_data_33[i];
        }
    }

    uint64_t party_bandwidth = Bandwidth();
    this->conn_[0]->WriteLong(party_bandwidth, false);
    this->conn_[1]->WriteLong(party_bandwidth, false);
    uint64_t total_bandwidth = party_bandwidth;
    total_bandwidth += (uint64_t)this->conn_[0]->ReadLong();
    total_bandwidth += (uint64_t)this->conn_[1]->ReadLong();

    conn_[0]->WriteLong(party_time, false);
    conn_[1]->WriteLong(party_time, false);
    uint64_t max_time = party_time;
    max_time = std::max(max_time, (uint64_t)conn_[0]->ReadLong());
    max_time = std::max(max_time, (uint64_t)conn_[1]->ReadLong());

    fprintf(stderr, "\n");
    fprintf(stderr, "Party Bandwidth(byte): %f\n", (double)party_bandwidth / iterations);
    fprintf(stderr, "Party Wallclock(microsec): %f\n", (double)party_time / iterations);
    fprintf(stderr, "Total Bandwidth(byte): %f\n", (double)total_bandwidth / iterations);
    fprintf(stderr, "Max Wallclock(microsec): %f\n", (double)max_time / iterations);
    fprintf(stderr, "\n");
}
