#include "dpf_oram.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const uint party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint64_t n, uint data_size) : Protocol(party, connections, rnd, prgs) {
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
    if (this->party_ == 3) {
        // conn 0:P1, 1:P2
        ssot->P3(this->data_size_);
        memset(v_out_13, 0, this->data_size_);
    } else if (this->party_ == 1) {
        // conn 0:P2, 1:P3
        uchar v_read_12[this->data_size_];
        xor_bytes(v_read_23[0], v_read_23[1], this->data_size_, v_read_12);

        uchar v_cache_12[this->data_size_];
        xor_bytes(v_cache_23[0], v_cache_23[1], this->data_size_, v_cache_12);

        uchar v_meta_12[1];
        xor_bytes(v_meta_23[0], v_meta_23[1], 1, v_meta_12);

        const uchar *u01[2] = {v_read_12, v_cache_12};
        ssot->P1(v_meta_12[0], u01, this->data_size_, v_out_13);
    } else if (this->party_ == 2) {
        // conn 0:P3, 1:P1
        const uchar *v01[2] = {v_read_23[0], v_cache_23[0]};
        ssot->P2(v_meta_23[0][0], v01, this->data_size_, v_out_13);
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
        this->cache_ctr_ = 0;
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
    uchar *tmp[2];
    for (uint64_t i = 0; i < this->n_; i++) {
        for (uint j = 0; j < 2; j++) {
            tmp[j] = this->read_array_23_[j][i];
        }
        this->ShareTwoThird(this->write_array_13_[i], this->data_size_, tmp);
        memset(this->write_array_13_[i], 0, this->data_size_);
    }
}

void DPFORAM::PrintMetadata() {
    std::cout << "===================" << std::endl;
    std::cout << "party_: " << this->party_ << std::endl;
    std::cout << "tau_: " << this->tau_ << std::endl;
    std::cout << "n_: " << this->n_ << std::endl;
    std::cout << "data_size_: " << this->data_size_ << std::endl;
    std::cout << "cache_ctr_: " << this->cache_ctr_ << std::endl;
    std::cout << "read_array_23_: " << (this->read_array_23_ != NULL) << std::endl;
    std::cout << "write_array_: " << (this->write_array_13_ != NULL) << std::endl;
    std::cout << "cache_23_: " << (this->cache_23_ != NULL) << std::endl;
    std::cout << "position_map_: " << (this->position_map_ != NULL) << std::endl;
    std::cout << "===================\n"
              << std::endl;
}

void DPFORAM::Test(uint iter) {
    uint64_t party_wc = 0;
    uint64_t wc;

    PrintMetadata();

    bool isRead = false;
    uint64_t range = 1ULL << (this->log_block_ct_ + this->tau_);
    uint64_t addr_23[2] = {10, 10};
    uchar *rec_23[2];
    uchar *new_rec_23[2];
    for (uint i = 0; i < 2; i++) {
        rec_23[i] = new uchar[this->next_log_n_size_];
        new_rec_23[i] = new uchar[this->next_log_n_size_];
        memset(rec_23[i], 0, this->next_log_n_size_);
        memset(new_rec_23[i], 0, this->next_log_n_size_);
    }
    uchar rec_exp[this->next_log_n_size_];
    memset(rec_exp, 0, this->next_log_n_size_ * sizeof(uchar));
    if (strcmp(party_, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        this->conn_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(party_, "debbie") == 0) {
        addr_23[1] = this->conn_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(party_, "eddie") == 0) {
            this->rnd_->GenerateBlock(new_rec_23[0], this->next_log_n_size_);
            this->conn_[0]->Write(new_rec_23[0], this->next_log_n_size_, false);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[this->next_log_n_size_];
            conn_[0]->Read(rec_out, this->next_log_n_size_);
            xor_bytes(rec_out, rec_23[0], this->next_log_n_size_, rec_out);
            xor_bytes(rec_out, rec_23[1], this->next_log_n_size_, rec_out);

            if (memcmp(rec_exp, rec_out, this->next_log_n_size_) == 0) {
                std::cout << "addr=" << addr_23[0] << ", t=" << t << ": Pass"
                          << std::endl;
            } else {
                std::cerr << "addr=" << addr_23[0] << ", t=" << t
                          << ": Fail !!!" << std::endl;
            }

            memcpy(rec_exp, new_rec_23[0], this->next_log_n_size_);
        } else if (strcmp(party_, "debbie") == 0) {
            conn_[1]->Read(new_rec_23[1], this->next_log_n_size_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            conn_[1]->Write(rec_23[0], this->next_log_n_size_, false);
        } else if (strcmp(party_, "charlie") == 0) {
            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;
        } else {
            std::cout << "Incorrect party: " << party_ << std::endl;
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

    conn_[0]->WriteLong(party_wc, false);
    conn_[1]->WriteLong(party_wc, false);
    uint64_t max_wc = party_wc;
    max_wc = std::max(max_wc, (uint64_t)conn_[0]->ReadLong());
    max_wc = std::max(max_wc, (uint64_t)conn_[1]->ReadLong());

    std::cout << std::endl;
    std::cout << "Party Bandwidth(byte): " << (party_band / iter) << std::endl;
    std::cout << "Party Wallclock(microsec): " << (party_wc / iter)
              << std::endl;
    std::cout << "Total Bandwidth(byte): " << (total_band / iter) << std::endl;
    std::cout << "Max Wallclock(microsec): " << (max_wc / iter) << std::endl;
    std::cout << std::endl;
}
