#include "dpf_oram.h"

#include <assert.h>
#include <omp.h>

#include <iostream>

#include "util.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const char *party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint n, uint data_size) : Protocol(party, connections, rnd, prgs) {
    this->n_ = n;
    this->data_size_ = data_size;

    this->tau_ = tau;
    // this->data_per_block_ = 1 << this->tau_;

    // this->block_ct_ = (this->data_ct_ + this->data_per_block_ - 1) / this->data_per_block_;
    // this->block_ct_size_ = (this->block_ct_ + 7) / 8;

    // this->log_block_ct_ = ceil(log2(this->block_ct_));
    // this->log_block_ct_size_ = (this->log_block_ct_ + 7) / 8;

    this->is_first_ = this->n_ <= 2;
    // first level position map, can be contained by two blocks.

    for (uint i = 0; i < 2; i++) {
        this->InitArray(this->read_array_[i]);
        this->InitArray(this->read_cache_[i]);
    }
    this->InitArray(this->write_array_);
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
    if (!this->is_first_) {
        delete this->position_map_;
        this->DeleteArray(this->read_cache_[0]);
        this->DeleteArray(this->read_cache_[1]);
        this->DeleteArray(this->write_array_);
    }
    this->DeleteArray(this->read_array_[0]);
    this->DeleteArray(this->read_array_[1]);
}

void DPFORAM::InitCacheCtr() {
    this->read_cache_ctr_ = 1;
}

// private
void DPFORAM::BlockPIR(const uint64_t index_23[2],
                       const uchar *const *const array_23[2], uint64_t array_size, uchar *block_23[2],
                       uchar *dpf_out[2]) {
    uchar *keys[2];
    uint key_size = this->fss_.Gen(index_23[0] ^ index_23[1], this->log_block_ct_, keys);

    this->conn_[0]->Write(keys[0], key_size);
    this->conn_[1]->Write(keys[1], key_size);

    this->conn_[0]->Read(keys[1], key_size);
    this->conn_[1]->Read(keys[0], key_size);

    uint quo = this->data_size_ / 16;  // 16 bytes = 128 bits
    uint reminder = this->data_size_ % 16;

    memset(block_23[0], 0, this->data_size_);

    if (omp_get_max_threads() == 1) {
        for (uint i = 0; i < 2; i++) {
            // this->fss_.EvalAll(keys[i], this->pos_log_n_, index_23[i], dpf_out[i]);
            this->fss_.EvalAll(keys[i], this->log_block_ct_, dpf_out[i]);
            for (uint64_t j = 0; j < array_size; j++) {
                // if (dpf_out[i][j])
                {
                    // set_xor_128(array_23[i][j], quo, rem, block_23[0]);
                    select_xor_128(array_23[i][j], dpf_out[i][j], quo, reminder, block_23[0]);
                }
            }
        }
    } else {
#pragma omp parallel
        {
#pragma omp for
            for (uint i = 0; i < 2; i++) {
                // this->fss_.EvalAll(keys[i], this->pos_log_n_, index_23[i], dpf_out[i]);
                this->fss_.EvalAll(keys[i], this->log_block_ct_, dpf_out[i]);
            }

            uchar tmp[data_size_];
            memset(tmp, 0, data_size_ * sizeof(uchar));
#pragma omp for collapse(2)
            for (uint i = 0; i < 2; i++) {
                for (uint64_t j = 0; j < array_size; j++) {
                    //					if (dpf_out[i][j])
                    {
                        //						set_xor_128(array_23[i][j], quo, rem, tmp);
                        select_xor_128(array_23[i][j], dpf_out[i][j], quo, reminder, tmp);
                    }
                }
            }
#pragma omp critical
            {
                set_xor_128(tmp, quo, reminder, block_23[0]);
            }
        }
    }

    this->conn_[0]->Write(block_23[0], data_size_);
    this->conn_[1]->Read(block_23[1], data_size_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::DataPIR(const uint index_23[2], const uchar *const block_23[2],
                      uchar *data_23[2]) {
    uchar *keys[2];
    uint key_size = this->fss_.Gen(index_23[0] ^ index_23[1], this->tau_, keys);
    this->conn_[0]->Write(keys[0], key_size);
    this->conn_[1]->Write(keys[1], key_size);
    this->conn_[0]->Read(keys[1], key_size);
    this->conn_[1]->Read(keys[0], key_size);

    memset(data_23[0], 0, this->next_log_n_size_);
    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[this->data_per_block_];
        this->fss_.EvalAll(keys[i], this->tau_, dpf_out);
        for (uint j = 0; j < data_per_block_; j++) {
            if (dpf_out[j ^ index_23[i]]) {
                cal_xor(data_23[0], block_23[i] + j * next_log_n_size_,
                        next_log_n_size_, data_23[0]);
            }
        }
    }

    this->conn_[0]->Write(data_23[0], next_log_n_size_);
    this->conn_[1]->Read(data_23[1], next_log_n_size_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::UpdateWriteArray(const uchar *const delta_block_23[2],
                               const uchar *const dpf_out[2]) {
    uint quo = this->data_size_ / 16;
    uint rem = this->data_size_ % 16;
#pragma omp parallel for
    for (uint64_t j = 0; j < n_; j++) {
        for (uint i = 0; i < 2; i++) {
            select_xor_128(delta_block_23[i], dpf_out[i][j], quo, rem, this->write_array_[j]);
        }
    }
}

void DPFORAM::AppendCache(const uchar *const block_23[2],
                          const uchar *const delta_block_23[2]) {
    for (uint i = 0; i < 2; i++) {
        cal_xor(block_23[i], delta_block_23[i], this->data_size_, this->read_cache_[i][this->read_cache_ctr_]);
    }
    this->read_cache_ctr_++;
    if (this->read_cache_ctr_ == this->n_) {
        this->InitCacheCtr();
        this->WriteArrayToReadArray();
        // this->position_map_->Init();
    }
}

// TODO: buffered read/write
void DPFORAM::WriteArrayToReadArray() {
    if (this->is_first_) return;
    for (uint64_t i = 0; i < this->n_; i++) {
        memcpy(this->read_array_[0][i], this->write_array_[i], this->data_size_);
    }
    for (uint64_t i = 0; i < this->n_; i++) {
        this->conn_[0]->Write(this->write_array_[i], this->data_size_);
        //		cons[0]->fwrite(wom[i], DBytes);
    }
    //	cons[0]->Flush();
    for (uint64_t i = 0; i < this->n_; i++) {
        this->conn_[1]->Read(this->read_array_[1][i], this->data_size_);
        //		cons[1]->fread(rom[1][i], DBytes);
    }
}

void DPFORAM::Access(const uint64_t index_23[2], const uchar *const new_data_23[2],
                     bool is_read, uchar *data_23[2]) {
    uint64_t block_index_23[2];
    uint data_index_23[2];
    for (uint i = 0; i < 2; i++) {
        block_index_23[i] = index_23[i] / this->data_per_block_;
        data_index_23[i] = index_23[i] % this->data_per_block_;
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
        DataPIR(data_index_23, block_23, data_23);

        for (uint i = 0; i < 2; i++) {
            if (is_read) {
                memset(delta_data_23[i], 0, this->next_log_n_size_);
            } else {
                cal_xor(data_23[i], new_data_23[i], this->next_log_n_size_,
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
                cal_xor(read_array_[i][j], delta_read_array_23[i] + j * this->data_size_, this->data_size_,
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
        long_to_bytes(this->read_cache_ctr_, new_cache_index, this->log_block_ct_size_);
        new_cache_index[0] = 1;  // + 1 to the ctr?

        uchar *new_cache_index_23[2];
        for (uint i = 0; i < 2; i++) {
            new_cache_index_23[i] = new_cache_index;
        }

        this->position_map_->Access(block_index_23, new_cache_index_23, false, cache_index_23);

        uint64_t mask2 = this->n_ - 1;
        uint64_t cache_block_index_23[2];
        for (uint i = 0; i < 2; i++) {
            cache_block_index_23[i] = bytes_to_long(cache_index_23[i], this->log_block_ct_size_) & mask2;
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
        BlockPIR(cache_block_index_23, this->read_cache_, this->read_cache_ctr_, cache_block_23,
                 cache_dpf_out);

        // uchar indicator_23[2] = {cache_index_23[0][0], cache_index_23[1][0]};
        // obliv_select(read_array_block_23, stash_block_23, indicator_23, block_23);

        DataPIR(data_index_23, block_23, data_23);
        uchar *delta_data_23[2];
        uchar *delta_block_23[2];
        for (uint i = 0; i < 2; i++) {
            delta_data_23[i] = new uchar[this->next_log_n_size_];
            if (is_read) {
                memset(delta_data_23[i], 0, this->next_log_n_size_);
            } else {
                cal_xor(data_23[i], new_data_23[i], this->next_log_n_size_, delta_data_23[i]);
            }
            delta_block_23[i] = new uchar[this->data_size_];
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        UpdateWriteArray(delta_block_23, read_array_dpf_out);
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
//         il.runE(idx_23[0] ^ idx_23[1], L1, numChunk, chunkBytes);
//         prgs[0].GenerateBlock(delta_array_23[0], arrayBytes);
//         prgs[1].GenerateBlock(delta_array_23[1], arrayBytes);
//     } else if (strcmp(party, "debbie") == 0) {
//         il.runD(idx_23[0], delta_23[0], numChunk, chunkBytes,
//                 delta_array_23[0]);
//         prgs[1].GenerateBlock(delta_array_23[1], arrayBytes);
//         cal_xor(delta_array_23[0], delta_array_23[1], arrayBytes,
//                 delta_array_23[0]);
//         cons[0]->write(delta_array_23[0], arrayBytes);
//         uchar tmp[arrayBytes];
//         cons[0]->read(tmp, arrayBytes);
//         cal_xor(delta_array_23[0], tmp, arrayBytes, delta_array_23[0]);
//     } else if (strcmp(party, "charlie") == 0) {
//         il.runC(numChunk, chunkBytes, delta_array_23[1]);
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
    std::cout << "Party: " << kParty << std::endl;
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
    std::cout << "Stash counter: " << this->read_cache_ctr_ << std::endl;
    std::cout << "ROM: " << (this->read_array_ != NULL) << std::endl;
    std::cout << "WOM: " << (this->write_array_ != NULL) << std::endl;
    std::cout << "stash: " << (this->read_cache_ != NULL) << std::endl;
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
    if (strcmp(kParty, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        this->conn_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(kParty, "debbie") == 0) {
        addr_23[1] = this->conn_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(kParty, "eddie") == 0) {
            this->rnd_->GenerateBlock(new_rec_23[0], this->next_log_n_size_);
            this->conn_[0]->Write(new_rec_23[0], this->next_log_n_size_, false);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[this->next_log_n_size_];
            conn_[0]->Read(rec_out, this->next_log_n_size_);
            cal_xor(rec_out, rec_23[0], this->next_log_n_size_, rec_out);
            cal_xor(rec_out, rec_23[1], this->next_log_n_size_, rec_out);

            if (memcmp(rec_exp, rec_out, this->next_log_n_size_) == 0) {
                std::cout << "addr=" << addr_23[0] << ", t=" << t << ": Pass"
                          << std::endl;
            } else {
                std::cerr << "addr=" << addr_23[0] << ", t=" << t
                          << ": Fail !!!" << std::endl;
            }

            memcpy(rec_exp, new_rec_23[0], this->next_log_n_size_);
        } else if (strcmp(kParty, "debbie") == 0) {
            conn_[1]->Read(new_rec_23[1], this->next_log_n_size_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            conn_[1]->Write(rec_23[0], this->next_log_n_size_, false);
        } else if (strcmp(kParty, "charlie") == 0) {
            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;
        } else {
            std::cout << "Incorrect party: " << kParty << std::endl;
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

void DPFORAM::InitArray(uchar **&array) {
    array = new uchar *[this->block_ct_];
    for (uint i = 0; i < this->block_ct_; i++) {
        array[i] = new uchar[this->data_size_]();  // init to zero
    }
}

void DPFORAM::DeleteArray(uchar **array) {
    for (uint i = 0; i < this->block_ct_; i++) {
        delete[] array[i];
    }
    delete[] array;
}