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
    this->is_last_ = is_last;                                            // is last level ORAM
    this->tau_ = is_last ? std::max(4 - (int)log2(data_size), 0) : tau;  // this value was 5, instead of 4. we use uint128, so it should be 4.
    // 2^4 = 16, a dpf block can store 16 bytes = 128 bits
    // TODO data_size is 0 in position maps ???
    // data_size    blocks      data per block
    //              n           2^tau
    // log n        n / 2^tau
    this->log_n_ = (log_n <= this->tau_ || !is_last) ? log_n : (log_n - this->tau_);
    this->data_per_block_ = 1 << this->tau_;
    this->log_n_size_ = (this->log_n_ + 7) / 8 + 1;  // size of this->log_n_ in bytes + 1
    this->next_log_n_ = is_last ? 0 : log_n + tau;
    this->next_log_n_size_ = is_last ? data_size : (this->next_log_n_ + 7) / 8 + 1;  // size of this->next_log_n_ in bytes + 1
    this->data_size_ = this->next_log_n_size_ * this->data_per_block_;
    this->n_ = 1ul << this->log_n_;
    this->is_first_ = this->log_n_ < 2 * tau;  // is first level ORAM

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
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, tau, this->log_n_ - tau, 0,
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
    for (unsigned long i = 0; i < this->n_; i++) {
        memset(mem[i], 0, this->data_size_);
    }
}

// private
void DPFORAM::BlockPIR(const unsigned long addr_23[2],
                       const uchar *const *const mem_23[2], unsigned long mem_size, uchar *block_23[2],
                       uchar *fss_out[2]) {
    uchar *keys[2];
    uint key_bytes = fss_.Gen(addr_23[0] ^ addr_23[1], this->log_n_, keys);
    this->connnections_[0]->Write(keys[0], key_bytes);
    this->connnections_[1]->Write(keys[1], key_bytes);
    this->connnections_[0]->Read(keys[1], key_bytes);
    this->connnections_[1]->Read(keys[0], key_bytes);

    uint quo = this->data_size_ / 16;
    uint reminder = this->data_size_ % 16;
    memset(block_23[0], 0, this->data_size_);

    if (omp_get_max_threads() == 1) {
        for (uint i = 0; i < 2; i++) {
            this->fss_.EvalAllWithPerm(keys[i], log_n_, addr_23[i], fss_out[i]);
            for (unsigned long j = 0; j < mem_size; j++) {
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
                this->fss_.EvalAllWithPerm(keys[i], log_n_, addr_23[i], fss_out[i]);
            }

            uchar tmp[data_size_];
            memset(tmp, 0, data_size_ * sizeof(uchar));
#pragma omp for collapse(2)
            for (uint i = 0; i < 2; i++) {
                for (unsigned long j = 0; j < mem_size; j++) {
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

    connnections_[0]->Write(block_23[0], data_size_);
    connnections_[1]->Read(block_23[1], data_size_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::RecPIR(const uint index_23[2], const uchar *const block_23[2],
                     uchar *rec_23[2]) {
    uchar *keys[2];
    uint keyBytes = fss_.Gen(index_23[0] ^ index_23[1], tau_, keys);
    connnections_[0]->Write(keys[0], keyBytes);
    connnections_[1]->Write(keys[1], keyBytes);
    connnections_[0]->Read(keys[1], keyBytes);
    connnections_[1]->Read(keys[0], keyBytes);

    memset(rec_23[0], 0, this->next_log_n_size_);
    for (uint i = 0; i < 2; i++) {
        uchar fss_out[this->data_per_block_];
        this->fss_.EvalAll(keys[i], this->tau_, fss_out);
        for (uint j = 0; j < data_per_block_; j++) {
            if (fss_out[j ^ index_23[i]]) {
                cal_xor(rec_23[0], block_23[i] + j * next_log_n_size_,
                        next_log_n_size_, rec_23[0]);
            }
        }
    }

    connnections_[0]->Write(rec_23[0], next_log_n_size_);
    connnections_[1]->Read(rec_23[1], next_log_n_size_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::UpdateWOM(const uchar *const delta_block_23[2],
                        const uchar *const fss_out[2]) {
    uint quo = this->data_size_ / 16;
    uint rem = this->data_size_ % 16;
#pragma omp parallel for
    for (unsigned long j = 0; j < n_; j++) {
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
    for (unsigned long i = 0; i < n_; i++) {
        memcpy(this->read_array_[0][i], this->write_array_[i], this->data_size_);
    }
    for (unsigned long i = 0; i < n_; i++) {
        this->connnections_[0]->Write(this->write_array_[i], this->data_size_);
        //		cons[0]->fwrite(wom[i], DBytes);
    }
    //	cons[0]->Flush();
    for (unsigned long i = 0; i < n_; i++) {
        this->connnections_[1]->Read(this->read_array_[1][i], this->data_size_);
        //		cons[1]->fread(rom[1][i], DBytes);
    }
}

void DPFORAM::Access(const unsigned long addr_23[2], const uchar *const new_rec_23[2],
                     bool is_read, uchar *rec_23[2]) {
    unsigned long addr_pre_23[2];
    uint addr_suf_23[2];
    for (uint i = 0; i < 2; i++) {
        addr_pre_23[i] = addr_23[i] >> this->tau_;
        addr_suf_23[i] = (uint)addr_23[i] & (1u << this->tau_ - 1u);
    }

    if (is_first_) {
        uchar *block_23[2];
        uchar *fss_out[2];
        uchar *delta_rec_23[2];
        uchar *delta_block_23[2];
        uchar *delta_rom_23[2];
        for (uint i = 0; i < 2; i++) {
            block_23[i] = new uchar[this->data_size_];
            fss_out[i] = new uchar[this->n_];
            delta_rec_23[i] = new uchar[this->next_log_n_size_];
            delta_block_23[i] = new uchar[this->data_size_];
            delta_rom_23[i] = new uchar[this->n_ * this->data_size_];
        }
        BlockPIR(addr_pre_23, this->read_array_, this->n_, block_23, fss_out);
        RecPIR(addr_suf_23, block_23, rec_23);

        for (uint i = 0; i < 2; i++) {
            if (is_read) {
                memset(delta_rec_23[i], 0, this->next_log_n_size_);
            } else {
                cal_xor(rec_23[i], new_rec_23[i], this->next_log_n_size_,
                        delta_rec_23[i]);
            }
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        // uint int_addr_pre_23[2] = {(uint)addr_pre_23[0], (uint)addr_pre_23[1]};
        // gen_delta_array(int_addr_pre_23, (uint)N, DBytes, delta_block_23,
        //                 delta_rom_23);

        for (uint i = 0; i < 2; i++) {
            for (unsigned long j = 0; j < n_; j++) {
                cal_xor(read_array_[i][j], delta_rom_23[i] + j * this->data_size_, this->data_size_,
                        read_array_[i][j]);
            }
        }

        for (uint i = 0; i < 2; i++) {
            delete[] block_23[i];
            delete[] fss_out[i];
            delete[] delta_rec_23[i];
            delete[] delta_block_23[i];
            delete[] delta_rom_23[i];
        }

        return;
    }

    ////////////////////////////////////////////////////////////////////

    uchar new_stash_ptr[this->log_n_size_];
    long_to_bytes(this->read_cache_ctr_, new_stash_ptr, this->log_n_size_);
    new_stash_ptr[0] = 1;

    uchar *stash_ptr_23[2];
    uchar *new_stash_ptr_23[2];
    for (uint i = 0; i < 2; i++) {
        stash_ptr_23[i] = new uchar[this->log_n_size_];
        new_stash_ptr_23[i] = new_stash_ptr;
    }
    position_map_->Access(addr_pre_23, new_stash_ptr_23, false, stash_ptr_23);

    unsigned long mask2 = n_ - 1;
    unsigned long stash_addr_pre_23[2];
    stash_addr_pre_23[0] = bytes_to_long(stash_ptr_23[0], this->log_n_size_) & mask2;
    stash_addr_pre_23[1] = bytes_to_long(stash_ptr_23[1], this->log_n_size_) & mask2;

    uchar *rom_block_23[2];
    uchar *stash_block_23[2];
    uchar *rom_fss_out[2];
    uchar *stash_fss_out[2];
    uchar *block_23[2];
    for (uint i = 0; i < 2; i++) {
        rom_block_23[i] = new uchar[this->data_size_];
        stash_block_23[i] = new uchar[this->data_size_];
        rom_fss_out[i] = new uchar[this->n_];
        stash_fss_out[i] = new uchar[this->n_];
        block_23[i] = new uchar[this->data_size_];
    }
    BlockPIR(addr_pre_23, this->read_array_, this->n_, rom_block_23, rom_fss_out);
    BlockPIR(stash_addr_pre_23, this->read_cache_, this->read_cache_ctr_, stash_block_23,
             stash_fss_out);

    uchar indicator_23[2] = {stash_ptr_23[0][0], stash_ptr_23[1][0]};
    // obliv_select(rom_block_23, stash_block_23, indicator_23, block_23);

    RecPIR(addr_suf_23, block_23, rec_23);
    uchar *delta_rec_23[2];
    uchar *delta_block_23[2];
    for (uint i = 0; i < 2; i++) {
        delta_rec_23[i] = new uchar[this->next_log_n_size_];
        if (is_read) {
            memset(delta_rec_23[i], 0, this->next_log_n_size_);
        } else {
            cal_xor(rec_23[i], new_rec_23[i], this->next_log_n_size_, delta_rec_23[i]);
        }
        delta_block_23[i] = new uchar[this->data_size_];
    }
    // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
    //                 delta_block_23);

    UpdateWOM(delta_block_23, rom_fss_out);
    AppendCache(block_23, delta_block_23);

    for (uint i = 0; i < 2; i++) {
        delete[] stash_ptr_23[i];
        delete[] rom_block_23[i];
        delete[] stash_block_23[i];
        delete[] rom_fss_out[i];
        delete[] stash_fss_out[i];
        delete[] block_23[i];
        delete[] delta_rec_23[i];
        delete[] delta_block_23[i];
    }
}

void DPFORAM::PrintMetadata() {
    std::cout << "===================" << std::endl;
    std::cout << "Party: " << kParty << std::endl;
    std::cout << "Last level: " << this->is_last_ << std::endl;
    std::cout << "First level: " << this->is_first_ << std::endl;
    std::cout << "tau: " << this->tau_ << std::endl;
    std::cout << "2^tau: " << this->data_per_block_ << std::endl;
    std::cout << "logN: " << this->log_n_ << std::endl;
    std::cout << "N: " << this->n_ << std::endl;
    std::cout << "logNBytes: " << this->log_n_size_ << std::endl;
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
    unsigned long party_wc = 0;
    unsigned long wc;

    PrintMetadata();

    bool isRead = false;
    unsigned long range = 1ul << (this->log_n_ + this->tau_);
    unsigned long addr_23[2] = {10, 10};
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
        this->connnections_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(kParty, "debbie") == 0) {
        addr_23[1] = this->connnections_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(kParty, "eddie") == 0) {
            this->rnd_->GenerateBlock(new_rec_23[0], this->next_log_n_size_);
            this->connnections_[0]->Write(new_rec_23[0], this->next_log_n_size_, false);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[this->next_log_n_size_];
            connnections_[0]->Read(rec_out, this->next_log_n_size_);
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
            connnections_[1]->Read(new_rec_23[1], this->next_log_n_size_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            connnections_[1]->Write(rec_23[0], this->next_log_n_size_, false);
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

    unsigned long party_band = Bandwidth();
    this->connnections_[0]->WriteLong(party_band, false);
    this->connnections_[1]->WriteLong(party_band, false);
    unsigned long total_band = party_band;
    total_band += (unsigned long)this->connnections_[0]->ReadLong();
    total_band += (unsigned long)this->connnections_[1]->ReadLong();

    connnections_[0]->WriteLong(party_wc, false);
    connnections_[1]->WriteLong(party_wc, false);
    unsigned long max_wc = party_wc;
    max_wc = std::max(max_wc, (unsigned long)connnections_[0]->ReadLong());
    max_wc = std::max(max_wc, (unsigned long)connnections_[1]->ReadLong());

    std::cout << std::endl;
    std::cout << "Party Bandwidth(byte): " << (party_band / iter) << std::endl;
    std::cout << "Party Wallclock(microsec): " << (party_wc / iter)
              << std::endl;
    std::cout << "Total Bandwidth(byte): " << (total_band / iter) << std::endl;
    std::cout << "Max Wallclock(microsec): " << (max_wc / iter) << std::endl;
    std::cout << std::endl;
}
