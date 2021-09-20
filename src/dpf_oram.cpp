#include "dpf_oram.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const uint party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
                 uint64_t n, uint data_size, uint tau) : Protocol(party, connections, rnd, prgs) {
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

    if (this->n_ < 2) {
        this->position_map_ = NULL;
    } else {
        uint pos_n = this->n_ >> this->tau_;
        uint pos_data_per_block = 1ULL << this->tau_;
        uint pos_data_size = (((uint)ceil(log2(this->n_)) + 7) / 8 + 1) * pos_data_per_block;
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, tau, pos_n, pos_data_size);
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

void DPFORAM::PIR(const uchar **array_23[2], const uint64_t n, const uint data_size, const uint64_t index_23[2], uchar *v_out) {
    uint log_n = ceil(log2(n));

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    uchar dpf_out[n];
    this->fss_.EvalAll(query_23[0], log_n, dpf_out);
    if (dpf_out[index_23[0] ^ index_23[1]] == 0) {
        // Make sure query[0] has one more 1 than query[1].
        std::swap(query_23[0], query_23[1]);
    }

    this->conn_[0]->Write(query_23[0], query_size);
    this->conn_[1]->Write(query_23[1], query_size);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    uchar *v[2];
    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[i], log_n, dpf_out);
        v[i] = new uchar[data_size]();
        for (uint j = 0; j < n; j++) {
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
}

void DPFORAM::PIW(uchar **array, const uint64_t n, const uint data_size, const uint64_t index_23[2], const uchar *v_delta) {
    uint log_n = ceil(log2(n));

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    uchar dpf_out[n];
    this->fss_.EvalAll(query_23[0], log_n, dpf_out);
    if (dpf_out[index_23[0] ^ index_23[1]] == 0) {
        // Make sure query[0] has one more 1 than query[1].
        std::swap(query_23[0], query_23[1]);
    }

    this->conn_[0]->Write(query_23[0], query_size);
    this->conn_[1]->Write(query_23[1], query_size);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[i], log_n, dpf_out);
        for (uint j = 0; j < n; j++) {
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
    this->ShareTwoThird(v_meta_13, 1, v_meta_23);

    SSOT *ssot = new SSOT(this->party_, this->conn_, this->rnd_, this->prgs_);
    if (this->party_ == 2) {
        // conn 0:P0, 1:P1
        ssot->P2(this->data_size_);
        memset(v_out_13, 0, this->data_size_);
    } else if (this->party_ == 0) {
        // conn 0:P1, 1:P2
        uchar v_read_12[this->data_size_];
        xor_bytes(v_read_23[0], v_read_23[1], this->data_size_, v_read_12);

        uchar v_cache_12[this->data_size_];
        xor_bytes(v_cache_23[0], v_cache_23[1], this->data_size_, v_cache_12);

        uchar v_meta_12[1];
        xor_bytes(v_meta_23[0], v_meta_23[1], 1, v_meta_12);

        const uchar *u01[2] = {v_read_12, v_cache_12};
        ssot->P0(v_meta_12[0], u01, this->data_size_, v_out_13);
    } else if (this->party_ == 1) {
        // conn 0:P2, 1:P0
        const uchar *v01[2] = {v_read_23[0], v_cache_23[0]};
        ssot->P1(v_meta_23[0][0], v01, this->data_size_, v_out_13);
    }

    delete ssot;
    for (uint i = 0; i < 2; i++) {
        delete[] v_read_23[i];
        delete[] v_cache_23[i];
        delete[] v_meta_23[i];
    }
}

void DPFORAM::ShareTwoThird(const uchar *v_in_13, const uint data_size, uchar *v_out_23[2]) {
    this->prgs_[0].GenerateBlock(v_out_23[0], data_size);
    this->prgs_[1].GenerateBlock(v_out_23[1], data_size);
    xor_bytes(v_in_13, v_out_23[0], data_size, v_out_23[1]);

    this->conn_[0]->Write(v_out_23[0], data_size);
    this->conn_[1]->Write(v_out_23[1], data_size);

    uchar tmp[data_size];
    this->conn_[0]->Read(tmp, data_size);
    xor_bytes(v_out_23[0], tmp, data_size, v_out_23[0]);
    this->conn_[1]->Read(tmp, data_size);
    xor_bytes(v_out_23[1], tmp, data_size, v_out_23[1]);
}

void DPFORAM::ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], uchar v_meta_13[1]) {
    uint data_size = ((uint)ceil(log2(this->n_)) + 7) / 8 + 1;
    uint data_per_block = 1U << this->tau_;

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

    uchar delta_block_13[this->position_map_->data_size_];
    xor_bytes(old_block_13, new_block_13, this->position_map_->data_size_, delta_block_13);

    this->position_map_->Write(block_index_23, delta_block_13);

    // append cache
    this->position_map_->AppendCache(new_block_13);

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
    // TODO: the case there's no position map.

    // read v_r from the read array.
    uchar v_read_13[this->data_size_];
    PIR(this->read_array_23_, this->n_, this->data_size_, index_23, v_read_13);

    // read the position map.
    uint64_t cache_index_23[2];
    uchar v_meta_13[1];
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, v_meta_13);
    } else {
        // TODO
    }

    // read the cache.
    uchar v_cache_13[this->data_size_];
    PIR(this->cache_23_, this->cache_ctr_, this->data_size_, cache_index_23, v_cache_13);

    // Use SSOT to select v_r and v_cache by is_used
    uchar v_out_13[this->data_size_];
    GetLatestData(v_read_13, v_cache_13, v_meta_13, v_out_13);
    this->ShareTwoThird(v_out_13, this->data_size_, v_out_23);
}

void DPFORAM::Write(const uint64_t index_23[2], const uchar *v_delta) {
    PIW(this->write_array_13_, this->n_, this->data_size_, index_23, v_delta);
}

void DPFORAM::AppendCache(const uchar *v_new_13) {
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
    fprintf(stderr, "n_: %" PRIu64 "\n", this->n_);
    fprintf(stderr, "data_size_: %u\n", this->data_size_);
    fprintf(stderr, "cache_ctr_: %" PRIu64 "\n", this->cache_ctr_);
    fprintf(stderr, "read_array_23_: %d\n", (this->read_array_23_ != NULL));
    fprintf(stderr, "write_array_13_: %d\n", (this->write_array_13_ != NULL));
    fprintf(stderr, "cache_23_: %d\n", (this->cache_23_ != NULL));
    fprintf(stderr, "position_map_: %d\n", (this->position_map_ != NULL));
    fprintf(stderr, "===================\n");
}

void DPFORAM::Test(uint iter) {
    uint64_t party_time = 0;
    uint64_t time;

    PrintMetadata();

    uint64_t range = 1ULL << (this->log_block_ct_ + this->tau_);
    uint64_t index_23[2] = {10, 10};
    uchar *rec_23[2];
    uchar *new_rec_23[2];
    for (uint i = 0; i < 2; i++) {
        rec_23[i] = new uchar[this->data_size_]();
        new_rec_23[i] = new uchar[this->data_size_]();
    }
    uchar rec_exp[this->data_size_];
    memset(rec_exp, 0, this->data_size_ * sizeof(uchar));
    if (this->party_ == 1) {
        // conn 0:P2, 1:P0
        index_23[0] = rand_uint64(range);
        this->conn_[0]->WriteLong(index_23[0], false);
    } else if (this->party_ == 2) {
        // conn 0:P0, 1:P1
        index_23[1] = this->conn_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (this->party_ == 1) {
            // conn 0:P2, 1:P0
            this->rnd_->GenerateBlock(new_rec_23[0], this->data_size_);
            this->conn_[0]->Write(new_rec_23[0], this->data_size_, false);

            time = timestamp();
            this->Access(index_23, new_rec_23, rec_23);
            party_time += timestamp() - time;

            uchar rec_out[this->data_size_];
            conn_[0]->Read(rec_out, this->data_size_);
            xor_bytes(rec_out, rec_23[0], this->data_size_, rec_out);
            xor_bytes(rec_out, rec_23[1], this->data_size_, rec_out);

            if (memcmp(rec_exp, rec_out, this->data_size_) == 0) {
                fprintf(stderr, "addr=%" PRIu64 ", t=%u: Pass\n", index_23[0], t);
            } else {
                fprintf(stderr, "addr=%" PRIu64 ", t=%u: Fail !!!\n", index_23[0], t);
            }

            memcpy(rec_exp, new_rec_23[0], this->data_size_);
        } else if (this->party_ == 2) {
            // conn 0:P0, 1:P1
            conn_[1]->Read(new_rec_23[1], this->data_size_);

            time = timestamp();
            this->Access(index_23, new_rec_23, rec_23);
            party_time += timestamp() - time;

            conn_[1]->Write(rec_23[0], this->next_log_n_size_, false);
        } else if (this->party_ == 0) {
            // conn 0:P1, 1:P2
            time = timestamp();
            this->Access(index_23, new_rec_23, rec_23);
            party_time += timestamp() - time;
        } else {
            fprintf(stderr, "Incorrect party: %d\n", this->party_);
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] rec_23[i];
        delete[] new_rec_23[i];
    }

    uint64_t party_band = Bandwidth();
    this->conn_[0]->WriteLong(party_band, false);
    this->conn_[1]->WriteLong(party_band, false);
    uint64_t total_band = party_band;
    total_band += (uint64_t)this->conn_[0]->ReadLong();
    total_band += (uint64_t)this->conn_[1]->ReadLong();

    conn_[0]->WriteLong(party_time, false);
    conn_[1]->WriteLong(party_time, false);
    uint64_t max_wc = party_time;
    max_wc = std::max(max_wc, (uint64_t)conn_[0]->ReadLong());
    max_wc = std::max(max_wc, (uint64_t)conn_[1]->ReadLong());

    fprintf(stderr, "\n");
    fprintf(stderr, "Party Bandwidth(byte): %f\n", party_band / iter);
    fprintf(stderr, "Party Wallclock(microsec): %f\n", party_time / iter);
    fprintf(stderr, "Total Bandwidth(byte): %f\n", total_band / iter);
    fprintf(stderr, "Max Wallclock(microsec): %f\n", max_wc / iter);
    fprintf(stderr, "\n");
}
