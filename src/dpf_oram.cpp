#include "dpf_oram.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const uint party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint64_t n, uint data_size) : Protocol(party, connections, rnd, prgs) {
    this->n_ = n;
    this->data_size_ = data_size;

    this->tau_ = tau;

    this->is_first_ = this->n_ <= 2;
    // first level position map, can be contained by two blocks.

    for (uint i = 0; i < 2; i++) {
        this->InitArray(this->read_array_[i]);
        this->InitArray(this->cache_[i]);
        this->InitArray(this->write_array_[i]);
    }
    this->InitCacheCtr();

    if (this->is_first_) {
        this->position_map_ = NULL;
    } else {
        uint pos_n = this->n_ >> this->tau_;
        uint pos_data_size = ceil(log2(this->n_)) * (1U << this->tau_);
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, tau, pos_n, pos_data_size);
    }
}

DPFORAM::~DPFORAM() {
    if (this->position_map_ != NULL) delete this->position_map_;
    for (uint i = 0; i < 2; i++) {
        this->DeleteArray(this->read_array_[i]);
        this->DeleteArray(this->cache_[i]);
        this->DeleteArray(this->write_array_[i]);
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

void DPFORAM::InitCacheCtr() {
    this->cache_ctr_ = 1;
}

void DPFORAM::CheckPositionMap(const uchar *const rom_block_23[2],
                               const uchar *const stash_block_23[2], const uchar indicator_23[2],
                               uchar *block_23[2]) {
    SSOT *ssot = new SSOT(this->party, this->conn_, this->rnd_, this->prgs_);
    // SSOT ot(this->party, this->conn_, this->rnd_, this->prgs_);
    if (this->party == 3) {
        // P3
        ssot->P3(this->data_size_);
        this->prgs_[0].GenerateBlock(block_23[0], this->data_size_);
        this->prgs_[1].GenerateBlock(block_23[1], this->data_size_);
    } else if (this->party == 1) {
        // P1
        int b0 = indicator_23[1] & 1;
        const uchar *u01[2] = {rom_block_23[1], stash_block_23[1]};
        ssot->P1(b0, u01, this->data_size_, block_23[0]);
        this->prgs_[1].GenerateBlock(block_23[1], this->data_size_);
        xor_bytes(block_23[0], block_23[1], this->data_size_, block_23[0]);
        this->conn_[0]->Write(block_23[0], this->data_size_);
        uchar tmp[this->data_size_];
        this->conn_[0]->Read(tmp, this->data_size_);
        xor_bytes(block_23[0], tmp, this->data_size_, block_23[0]);
    } else if (this->party == 2) {
        // P2
        uint b1 = (indicator_23[0] ^ indicator_23[1]) & 1;
        uchar *v01[2] = {new uchar[this->data_size_], new uchar[this->data_size_]};
        xor_bytes(rom_block_23[0], rom_block_23[1], this->data_size_, v01[0]);
        xor_bytes(stash_block_23[0], stash_block_23[1], this->data_size_, v01[1]);
        ssot->P2(b1, v01, this->data_size_, block_23[1]);
        this->prgs_[0].GenerateBlock(block_23[0], this->data_size_);
        xor_bytes(block_23[0], block_23[1], this->data_size_, block_23[1]);
        this->conn_[1]->Write(block_23[1], this->data_size_);
        uchar tmp[this->data_size_];
        this->conn_[1]->Read(tmp, this->data_size_);
        xor_bytes(block_23[1], tmp, this->data_size_, block_23[1]);
        delete[] v01[0];
        delete[] v01[1];
    }
    delete ssot;
}

void DPFORAM::PIR(const uchar **array[2], const uint64_t n, const uint data_size, const uint64_t index[2], uchar *output) {
    uint log_n = ceil(log2(n));

    uchar *query[2];
    uint query_size = this->fss_.Gen(index[0] ^ index[1], log_n, query);

    uchar dpf_out[n];
    this->fss_.EvalAll(query[0], log_n, dpf_out);
    if (dpf_out[index[0] ^ index[1]] == 0) {
        // Make sure query[0] has one more 1 than query[1].
        std::swap(query[0], query[1]);
    }
    this->conn_[0]->Write(query[0], query_size);
    this->conn_[1]->Write(query[1], query_size);

    this->conn_[0]->Read(query[1], query_size);
    this->conn_[1]->Read(query[0], query_size);

    uchar *v[2];
    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query[i], log_n, dpf_out);
        v[i] = new uchar[data_size]();
        for (uint j = 0; j < n; j++) {
            if (dpf_out[j ^ index[i]]) {
                xor_bytes(v[i], array[i][j], data_size, v[i]);
            }
        }
    }
    xor_bytes(v[0], v[1], data_size, output);

    for (uint i = 0; i < 2; i++) {
        delete[] v[i];
    }
}

void DPFORAM::PIW(const uchar *const delta_block_23[2],
                  const uchar *const dpf_out[2]) {
    uint quo = this->data_size_ / 16;
    uint rem = this->data_size_ % 16;
#pragma omp parallel for
    for (uint64_t j = 0; j < n_; j++) {
        for (uint i = 0; i < 2; i++) {
            select_xor_128(delta_block_23[i], dpf_out[i][j], quo, rem, this->write_array_);
        }
    }
}

void DPFORAM::AppendCache(const uchar *const block_23[2],
                          const uchar *const delta_block_23[2]) {
    for (uint i = 0; i < 2; i++) {
        xor_bytes(block_23[i], delta_block_23[i], this->data_size_, this->cache_[i][this->cache_ctr_]);
    }
    this->cache_ctr_++;
    if (this->cache_ctr_ == this->n_) {
        this->InitCacheCtr();
        this->Flush();
        // this->position_map_->Init();
    }
}

// TODO: buffered read/write
void DPFORAM::Flush() {
    if (this->is_first_) return;
    for (uint64_t i = 0; i < this->n_; i++) {
        memcpy(this->read_array_[0][i], this->write_array_[i], this->data_size_);
    }
    for (uint64_t i = 0; i < this->n_; i++) {
        this->conn_[0]->Write(this->write_array_, this->data_size_);
        //		cons[0]->fwrite(wom[i], DBytes);
    }
    //	cons[0]->Flush();
    for (uint64_t i = 0; i < this->n_; i++) {
        this->conn_[1]->Read(this->read_array_[1][i], this->data_size_);
        //		cons[1]->fread(rom[1][i], DBytes);
    }
}

void DPFORAM::ReadPositionMap(uint64_t cache_index[2]) {
    uchar *pos_input = new uchar[this->position_map_->data_size_]();
    uchar *pos_output = new uchar[this->position_map_->data_size_]();
    this->position_map_->Access(index, pos_input, true, pos_output);

    bytes_to_two_uint64(pos_output, this->position_map_->data_size_, cache_index);

    delete[] pos_input;
    delete[] pos_output;
}

void DPFORAM::Access(const uint64_t index[2], const uchar *new_data,
                     const bool read_only, uchar *output) {
    // read v_r from the read array.
    uchar *v_read = new uchar[this->data_size_];
    PIR(this->read_array_, this->n_, this->data_size_, index, v_read);

    // read the position map.
    uint64_t cache_indexes[2];
    ReadPositionMap(cache_indexes);

    // read the cache.
    // TODO: make sure each two party have same index.
    uchar *v_cache = new uchar[this->data_size_];
    PIR(this->cache_, this->cache_ctr_, this->data_size_, cache_indexes, v_cache);

    // write to the write array.

    uint64_t block_index_23[2];
    uint data_index_23[2];
    for (uint i = 0; i < 2; i++) {
        block_index_23[i] = index[i] / this->data_per_block_;
        data_index_23[i] = index[i] % this->data_per_block_;
    }

    if (is_first_) {
        // first level, no position map.
        uchar *block_23[2];
        uchar *dpf_out[2];
        uchar *delta_data_23[2];
        uchar *delta_block_23[2];
        uchar *delta_read_array_23[2];
        for (uint i = 0; i < 2; i++) {
            block_23[i] = new uchar[this->data_size_];
            dpf_out[i] = new uchar[this->n_];
            delta_data_23[i] = new uchar[this->next_log_n_size_];
            delta_block_23[i] = new uchar[this->data_size_];
            delta_read_array_23[i] = new uchar[this->n_ * this->data_size_];
        }
        BlockPIR(block_index_23, this->read_array_, this->n_, block_23, dpf_out);
        DataPIR(data_index_23, block_23, output);

        for (uint i = 0; i < 2; i++) {
            if (read_only) {
                memset(delta_data_23[i], 0, this->next_log_n_size_);
            } else {
                xor_bytes(output[i], new_data[i], this->next_log_n_size_,
                          delta_data_23[i]);
            }
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        // uint int_addr_pre_23[2] = {(uint)addr_pre_23[0], (uint)addr_pre_23[1]};
        // gen_delta_array(int_addr_pre_23, (uint)N, DBytes, delta_block_23,
        //                 delta_rom_23);

        for (uint i = 0; i < 2; i++) {
            for (uint64_t j = 0; j < this->n_; j++) {
                xor_bytes(read_array_[i][j], delta_read_array_23[i] + j * this->data_size_, this->data_size_,
                          read_array_[i][j]);
            }
        }

        for (uint i = 0; i < 2; i++) {
            delete[] block_23[i];
            delete[] dpf_out[i];
            delete[] delta_data_23[i];
            delete[] delta_block_23[i];
            delete[] delta_read_array_23[i];
        }
    } else {
        // not first round, need to access position map.
        uchar *cache_index_23[2];
        for (uint i = 0; i < 2; i++) {
            cache_index_23[i] = new uchar[this->log_block_ct_size_];
        }

        uchar new_cache_index[this->log_block_ct_size_];
        uint64_to_bytes(this->cache_ctr_, new_cache_index, this->log_block_ct_size_);
        new_cache_index[0] = 1;  // + 1 to the ctr?

        uchar *new_cache_index_23[2];
        for (uint i = 0; i < 2; i++) {
            new_cache_index_23[i] = new_cache_index;
        }

        this->position_map_->Access(block_index_23, new_cache_index_23, false, cache_index_23);

        uint64_t mask2 = this->n_ - 1;
        uint64_t cache_block_index_23[2];
        for (uint i = 0; i < 2; i++) {
            cache_block_index_23[i] = bytes_to_uint64(cache_index_23[i], this->log_block_ct_size_) & mask2;
        }

        uchar *read_array_block_23[2];
        uchar *cache_block_23[2];
        uchar *read_array_dpf_out[2];
        uchar *cache_dpf_out[2];
        uchar *block_23[2];
        for (uint i = 0; i < 2; i++) {
            read_array_block_23[i] = new uchar[this->data_size_];
            cache_block_23[i] = new uchar[this->data_size_];
            read_array_dpf_out[i] = new uchar[this->n_];
            cache_dpf_out[i] = new uchar[this->n_];
            block_23[i] = new uchar[this->data_size_];
        }

        BlockPIR(block_index_23, this->read_array_, this->n_, read_array_block_23, read_array_dpf_out);
        BlockPIR(cache_block_index_23, this->cache_, this->cache_ctr_, cache_block_23,
                 cache_dpf_out);

        // uchar indicator_23[2] = {cache_index_23[0][0], cache_index_23[1][0]};
        // obliv_select(read_array_block_23, stash_block_23, indicator_23, block_23);

        DataPIR(data_index_23, block_23, output);
        uchar *delta_data_23[2];
        uchar *delta_block_23[2];
        for (uint i = 0; i < 2; i++) {
            delta_data_23[i] = new uchar[this->next_log_n_size_];
            if (read_only) {
                memset(delta_data_23[i], 0, this->next_log_n_size_);
            } else {
                xor_bytes(output[i], new_data[i], this->next_log_n_size_, delta_data_23[i]);
            }
            delta_block_23[i] = new uchar[this->data_size_];
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        PIW(delta_block_23, read_array_dpf_out);
        AppendCache(block_23, delta_block_23);

        for (uint i = 0; i < 2; i++) {
            delete[] cache_index_23[i];
            delete[] read_array_block_23[i];
            delete[] cache_block_23[i];
            delete[] read_array_dpf_out[i];
            delete[] cache_dpf_out[i];
            delete[] block_23[i];
            delete[] delta_data_23[i];
            delete[] delta_block_23[i];
        }
    }
}

// void DPFORAM::GenDeltaArray(const uint idx_23[2], uint numChunk,
//                               uint chunkBytes, const uchar *const delta_23[2],
//                               uchar *delta_array_23[2]) {
//     uint arrayBytes = numChunk * chunkBytes;
//     insert_label il(party, cons, rnd, prgs);
//     if (strcmp(party, "eddie") == 0) {
//         uchar L1[chunkBytes];
//         cal_xor(delta_23[0], delta_23[1], chunkBytes, L1);
//         il.P2(idx_23[0] ^ idx_23[1], L1, numChunk, chunkBytes);
//         prgs[0].GenerateBlock(delta_array_23[0], arrayBytes);
//         prgs[1].GenerateBlock(delta_array_23[1], arrayBytes);
//     } else if (strcmp(party, "debbie") == 0) {
//         il.P3(idx_23[0], delta_23[0], numChunk, chunkBytes,
//                 delta_array_23[0]);
//         prgs[1].GenerateBlock(delta_array_23[1], arrayBytes);
//         cal_xor(delta_array_23[0], delta_array_23[1], arrayBytes,
//                 delta_array_23[0]);
//         cons[0]->write(delta_array_23[0], arrayBytes);
//         uchar tmp[arrayBytes];
//         cons[0]->read(tmp, arrayBytes);
//         cal_xor(delta_array_23[0], tmp, arrayBytes, delta_array_23[0]);
//     } else if (strcmp(party, "charlie") == 0) {
//         il.P1(numChunk, chunkBytes, delta_array_23[1]);
//         prgs[0].GenerateBlock(delta_array_23[0], arrayBytes);
//         cal_xor(delta_array_23[0], delta_array_23[1], arrayBytes,
//                 delta_array_23[1]);
//         cons[1]->write(delta_array_23[1], arrayBytes);
//         uchar tmp[arrayBytes];
//         cons[1]->read(tmp, arrayBytes);
//         cal_xor(delta_array_23[1], tmp, arrayBytes, delta_array_23[1]);
//     } else {
//     }
// }

void DPFORAM::PrintMetadata() {
    std::cout << "===================" << std::endl;
    std::cout << "Party: " << party << std::endl;
    std::cout << "Last level: " << this->is_last_ << std::endl;
    std::cout << "First level: " << this->is_first_ << std::endl;
    std::cout << "tau: " << this->tau_ << std::endl;
    std::cout << "2^tau: " << this->data_per_block_ << std::endl;
    std::cout << "logN: " << this->log_block_ct_ << std::endl;
    std::cout << "N: " << this->n_ << std::endl;
    std::cout << "logNBytes: " << this->log_block_ct_size_ << std::endl;
    std::cout << "next_log_n_: " << this->next_log_n_ << std::endl;
    std::cout << "nextLogNBytes: " << this->next_log_n_size_ << std::endl;
    std::cout << "DBytes: " << this->data_size_ << std::endl;
    std::cout << "Stash counter: " << this->cache_ctr_ << std::endl;
    std::cout << "ROM: " << (this->read_array_ != NULL) << std::endl;
    std::cout << "WOM: " << (this->write_array_ != NULL) << std::endl;
    std::cout << "stash: " << (this->cache_ != NULL) << std::endl;
    std::cout << "posMap: " << (this->position_map_ != NULL) << std::endl;
    std::cout << "===================\n"
              << std::endl;

    if (!this->is_first_) {
        this->position_map_->PrintMetadata();
    }
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
    if (strcmp(party, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        this->conn_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(party, "debbie") == 0) {
        addr_23[1] = this->conn_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(party, "eddie") == 0) {
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
        } else if (strcmp(party, "debbie") == 0) {
            conn_[1]->Read(new_rec_23[1], this->next_log_n_size_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            conn_[1]->Write(rec_23[0], this->next_log_n_size_, false);
        } else if (strcmp(party, "charlie") == 0) {
            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;
        } else {
            std::cout << "Incorrect party: " << party << std::endl;
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
