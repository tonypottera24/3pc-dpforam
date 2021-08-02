#include "dpf_oram.h"

#include <assert.h>
#include <omp.h>

#include <iostream>

#include "util.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const char *party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint log_n, uint data_size, bool is_last) : Protocol(party, connections, rnd, prgs) {
    this->is_last_ = is_last;  // is last level ORAM

    this->tau_ = is_last ? std::max(4 - (int)log2(data_size), 0) : tau;  // this value was 5, instead of 4. we use uint128, so it should be 4.
    // 2^4 = 16, a dpf block can store 16 bytes = 128 bits
    // TODO data_size is 0 in position maps ???
    // data_size    blocks              data per block
    // d            n                   2^tau
    // log n        log log n / 2^tau

    this->pos_log_n_ = (is_last && log_n > this->tau_) ? log_n : (log_n - this->tau_);
    this->pos_log_n_size_ = (this->pos_log_n_ + 7) / 8 + 1;  // size of this->pos_log_n_ in bytes + 1
    // TODO (x + 7) / 8 is equivalence to a ceil() call, why + 1 ?

    this->oram_log_n_ = is_last ? 0 : log_n + tau;
    this->oram_log_n_size_ = is_last ? data_size : (this->oram_log_n_ + 7) / 8 + 1;  // size of this->oram_log_n_ in bytes + 1

    this->data_per_block_ = 1 << this->tau_;
    this->data_size_ = this->oram_log_n_size_ * this->data_per_block_;

    this->n_ = 1ULL << this->pos_log_n_;

    this->is_first_ = this->pos_log_n_ < 2 * tau;
    // first level position map, can be contained by two blocks.

    this->InitMem(this->read_array_[0]);
    this->InitMem(this->read_array_[1]);
    if (this->is_first_) {
        // init: last round recursion, first level ORAM
        this->write_array_ = NULL;
        this->position_map_ = NULL;
    } else {
        this->InitMem(this->write_array_);
        this->InitMem(this->read_cache_[0]);
        this->InitMem(this->read_cache_[1]);
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, tau, this->pos_log_n_ - tau, 0,
                                          false);
    }
    this->InitCacheCtr();

    if (is_last) {
        this->Init();
    }
}

DPFORAM::~DPFORAM() {
    if (!this->is_first_) {
        delete this->position_map_;
        this->DeleteMem(this->read_cache_[0]);
        this->DeleteMem(this->read_cache_[1]);
        this->DeleteMem(this->write_array_);
    }
    this->DeleteMem(this->read_array_[0]);
    this->DeleteMem(this->read_array_[1]);
}

void DPFORAM::Init() {
    this->InitCacheCtr();
    this->SetMemZero(this->read_array_[0]);
    this->SetMemZero(this->read_array_[1]);
    this->SetMemZero(this->write_array_);
    if (!this->is_first_) {
        this->position_map_->Init();
    }
}

void DPFORAM::InitCacheCtr() {
    this->read_cache_ctr_ = 1;
}

void DPFORAM::InitMem(uchar **&mem) {
    mem = new uchar *[this->n_];
    for (uint i = 0; i < this->n_; i++) {
        mem[i] = new uchar[this->data_size_];
    }
}

void DPFORAM::DeleteMem(uchar **mem) {
    for (uint i = 0; i < this->n_; i++) {
        delete[] mem[i];
    }
    delete[] mem;
}

void DPFORAM::SetMemZero(uchar **mem) {
    if (mem == NULL) return;
#pragma omp parallel for
    for (uint64_t i = 0; i < this->n_; i++) {
        memset(mem[i], 0, this->data_size_);
    }
}

// private
void DPFORAM::BlockPIR(const uint64_t addr_23[2],
                       const uchar *const *const mem_23[2], uint64_t mem_size, uchar *block_23[2],
                       uchar *fss_out[2]) {
    uchar *keys[2];
    uint key_size = this->fss_.Gen(addr_23[0] ^ addr_23[1], this->pos_log_n_, keys);
    this->conn_[0]->Write(keys[0], key_size);
    this->conn_[1]->Write(keys[1], key_size);
    this->conn_[0]->Read(keys[1], key_size);
    this->conn_[1]->Read(keys[0], key_size);

    uint quo = this->data_size_ / 16;  // 16 bytes = 128 bits
    uint reminder = this->data_size_ % 16;
    memset(block_23[0], 0, this->data_size_);

    if (omp_get_max_threads() == 1) {
        for (uint i = 0; i < 2; i++) {
            this->fss_.EvalAllWithPerm(keys[i], this->pos_log_n_, addr_23[i], fss_out[i]);
            for (uint64_t j = 0; j < mem_size; j++) {
                // if (fss_out[i][j])
                {
                    // set_xor_128(mem_23[i][j], quo, rem, block_23[0]);
                    select_xor_128(mem_23[i][j], fss_out[i][j], quo, reminder, block_23[0]);
                }
            }
        }
    } else {
#pragma omp parallel
        {
#pragma omp for
            for (uint i = 0; i < 2; i++) {
                this->fss_.EvalAllWithPerm(keys[i], this->pos_log_n_, addr_23[i], fss_out[i]);
            }

            uchar tmp[data_size_];
            memset(tmp, 0, data_size_ * sizeof(uchar));
#pragma omp for collapse(2)
            for (uint i = 0; i < 2; i++) {
                for (uint64_t j = 0; j < mem_size; j++) {
                    //					if (fss_out[i][j])
                    {
                        //						set_xor_128(mem_23[i][j], quo, rem, tmp);
                        select_xor_128(mem_23[i][j], fss_out[i][j], quo, reminder, tmp);
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
                      uchar *rec_23[2]) {
    uchar *keys[2];
    uint key_size = this->fss_.Gen(index_23[0] ^ index_23[1], this->tau_, keys);
    this->conn_[0]->Write(keys[0], key_size);
    this->conn_[1]->Write(keys[1], key_size);
    this->conn_[0]->Read(keys[1], key_size);
    this->conn_[1]->Read(keys[0], key_size);

    memset(rec_23[0], 0, this->oram_log_n_size_);
    for (uint i = 0; i < 2; i++) {
        uchar fss_out[this->data_per_block_];
        this->fss_.EvalAll(keys[i], this->tau_, fss_out);
        for (uint j = 0; j < data_per_block_; j++) {
            if (fss_out[j ^ index_23[i]]) {
                cal_xor(rec_23[0], block_23[i] + j * oram_log_n_size_,
                        oram_log_n_size_, rec_23[0]);
            }
        }
    }

    this->conn_[0]->Write(rec_23[0], oram_log_n_size_);
    this->conn_[1]->Read(rec_23[1], oram_log_n_size_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::UpdateWriteArray(const uchar *const delta_block_23[2],
                               const uchar *const fss_out[2]) {
    uint quo = this->data_size_ / 16;
    uint rem = this->data_size_ % 16;
#pragma omp parallel for
    for (uint64_t j = 0; j < n_; j++) {
        for (uint i = 0; i < 2; i++) {
            select_xor_128(delta_block_23[i], fss_out[i][j], quo, rem, this->write_array_[j]);
        }
    }
}

void DPFORAM::AppendCache(const uchar *const block_23[2],
                          const uchar *const delta_block_23[2]) {
    for (uint i = 0; i < 2; i++) {
        cal_xor(block_23[i], delta_block_23[i], this->data_size_, this->read_cache_[i][read_cache_ctr_]);
    }
    this->read_cache_ctr_++;
    if (this->read_cache_ctr_ == this->n_) {
        this->InitCacheCtr();
        this->WOM2ROM();
        this->position_map_->Init();
    }
}

// TODO: buffered read/write
void DPFORAM::WOM2ROM() {
    if (this->is_first_) {
        return;
    }
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

void DPFORAM::Access(const uint64_t index_23[2], const uchar *const new_rec_23[2],
                     bool is_read, uchar *rec_23[2]) {
    uint64_t block_index_23[2];
    uint data_index_23[2];
    for (uint i = 0; i < 2; i++) {
        block_index_23[i] = index_23[i] / this->data_per_block_;
        data_index_23[i] = index_23[i] % this->data_per_block_;
    }

    if (is_first_) {
        // first level, no position map.
        uchar *block_23[2];
        uchar *fss_out[2];
        uchar *delta_data_23[2];
        uchar *delta_block_23[2];
        uchar *delta_read_array_23[2];
        for (uint i = 0; i < 2; i++) {
            block_23[i] = new uchar[this->data_size_];
            fss_out[i] = new uchar[this->n_];
            delta_data_23[i] = new uchar[this->oram_log_n_size_];
            delta_block_23[i] = new uchar[this->data_size_];
            delta_read_array_23[i] = new uchar[this->n_ * this->data_size_];
        }
        BlockPIR(block_index_23, this->read_array_, this->n_, block_23, fss_out);
        DataPIR(data_index_23, block_23, rec_23);

        for (uint i = 0; i < 2; i++) {
            if (is_read) {
                memset(delta_data_23[i], 0, this->oram_log_n_size_);
            } else {
                cal_xor(rec_23[i], new_rec_23[i], this->oram_log_n_size_,
                        delta_data_23[i]);
            }
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        // uint int_addr_pre_23[2] = {(uint)addr_pre_23[0], (uint)addr_pre_23[1]};
        // gen_delta_array(int_addr_pre_23, (uint)N, DBytes, delta_block_23,
        //                 delta_rom_23);

        for (uint i = 0; i < 2; i++) {
            for (uint64_t j = 0; j < n_; j++) {
                cal_xor(read_array_[i][j], delta_read_array_23[i] + j * this->data_size_, this->data_size_,
                        read_array_[i][j]);
            }
        }

        for (uint i = 0; i < 2; i++) {
            delete[] block_23[i];
            delete[] fss_out[i];
            delete[] delta_data_23[i];
            delete[] delta_block_23[i];
            delete[] delta_read_array_23[i];
        }
    } else {
        // not first round, need to access position map.
        uchar new_cache_ptr[this->pos_log_n_size_];
        long_to_bytes(this->read_cache_ctr_, new_cache_ptr, this->pos_log_n_size_);
        new_cache_ptr[0] = 1;

        uchar *cache_ptr_23[2];
        uchar *new_cache_ptr_23[2];
        for (uint i = 0; i < 2; i++) {
            cache_ptr_23[i] = new uchar[this->pos_log_n_size_];
            new_cache_ptr_23[i] = new_cache_ptr;
        }
        this->position_map_->Access(block_index_23, new_cache_ptr_23, false, cache_ptr_23);

        uint64_t mask2 = n_ - 1;
        uint64_t cache_addr_pre_23[2];
        cache_addr_pre_23[0] = bytes_to_long(cache_ptr_23[0], this->pos_log_n_size_) & mask2;
        cache_addr_pre_23[1] = bytes_to_long(cache_ptr_23[1], this->pos_log_n_size_) & mask2;

        uchar *rom_block_23[2];
        uchar *cache_block_23[2];
        uchar *rom_fss_out[2];
        uchar *cache_fss_out[2];
        uchar *block_23[2];
        for (uint i = 0; i < 2; i++) {
            rom_block_23[i] = new uchar[this->data_size_];
            cache_block_23[i] = new uchar[this->data_size_];
            rom_fss_out[i] = new uchar[this->n_];
            cache_fss_out[i] = new uchar[this->n_];
            block_23[i] = new uchar[this->data_size_];
        }
        BlockPIR(block_index_23, this->read_array_, this->n_, rom_block_23, rom_fss_out);
        BlockPIR(cache_addr_pre_23, this->read_cache_, this->read_cache_ctr_, cache_block_23,
                 cache_fss_out);

        uchar indicator_23[2] = {cache_ptr_23[0][0], cache_ptr_23[1][0]};
        // obliv_select(rom_block_23, stash_block_23, indicator_23, block_23);

        DataPIR(data_index_23, block_23, rec_23);
        uchar *delta_rec_23[2];
        uchar *delta_block_23[2];
        for (uint i = 0; i < 2; i++) {
            delta_rec_23[i] = new uchar[this->oram_log_n_size_];
            if (is_read) {
                memset(delta_rec_23[i], 0, this->oram_log_n_size_);
            } else {
                cal_xor(rec_23[i], new_rec_23[i], this->oram_log_n_size_, delta_rec_23[i]);
            }
            delta_block_23[i] = new uchar[this->data_size_];
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        UpdateWriteArray(delta_block_23, rom_fss_out);
        AppendCache(block_23, delta_block_23);

        for (uint i = 0; i < 2; i++) {
            delete[] cache_ptr_23[i];
            delete[] rom_block_23[i];
            delete[] cache_block_23[i];
            delete[] rom_fss_out[i];
            delete[] cache_fss_out[i];
            delete[] block_23[i];
            delete[] delta_rec_23[i];
            delete[] delta_block_23[i];
        }
    }
}

void DPFORAM::PrintMetadata() {
    std::cout << "===================" << std::endl;
    std::cout << "Party: " << kParty << std::endl;
    std::cout << "Last level: " << this->is_last_ << std::endl;
    std::cout << "First level: " << this->is_first_ << std::endl;
    std::cout << "tau: " << this->tau_ << std::endl;
    std::cout << "2^tau: " << this->data_per_block_ << std::endl;
    std::cout << "logN: " << this->pos_log_n_ << std::endl;
    std::cout << "N: " << this->n_ << std::endl;
    std::cout << "logNBytes: " << this->pos_log_n_size_ << std::endl;
    std::cout << "oram_log_n_: " << this->oram_log_n_ << std::endl;
    std::cout << "nextLogNBytes: " << this->oram_log_n_size_ << std::endl;
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
    uint64_t range = 1ULL << (this->pos_log_n_ + this->tau_);
    uint64_t addr_23[2] = {10, 10};
    uchar *rec_23[2];
    uchar *new_rec_23[2];
    for (uint i = 0; i < 2; i++) {
        rec_23[i] = new uchar[this->oram_log_n_size_];
        new_rec_23[i] = new uchar[this->oram_log_n_size_];
        memset(rec_23[i], 0, this->oram_log_n_size_);
        memset(new_rec_23[i], 0, this->oram_log_n_size_);
    }
    uchar rec_exp[this->oram_log_n_size_];
    memset(rec_exp, 0, this->oram_log_n_size_ * sizeof(uchar));
    if (strcmp(kParty, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        this->conn_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(kParty, "debbie") == 0) {
        addr_23[1] = this->conn_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(kParty, "eddie") == 0) {
            this->rnd_->GenerateBlock(new_rec_23[0], this->oram_log_n_size_);
            this->conn_[0]->Write(new_rec_23[0], this->oram_log_n_size_, false);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[this->oram_log_n_size_];
            conn_[0]->Read(rec_out, this->oram_log_n_size_);
            cal_xor(rec_out, rec_23[0], this->oram_log_n_size_, rec_out);
            cal_xor(rec_out, rec_23[1], this->oram_log_n_size_, rec_out);

            if (memcmp(rec_exp, rec_out, this->oram_log_n_size_) == 0) {
                std::cout << "addr=" << addr_23[0] << ", t=" << t << ": Pass"
                          << std::endl;
            } else {
                std::cerr << "addr=" << addr_23[0] << ", t=" << t
                          << ": Fail !!!" << std::endl;
            }

            memcpy(rec_exp, new_rec_23[0], this->oram_log_n_size_);
        } else if (strcmp(kParty, "debbie") == 0) {
            conn_[1]->Read(new_rec_23[1], this->oram_log_n_size_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            conn_[1]->Write(rec_23[0], this->oram_log_n_size_, false);
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
