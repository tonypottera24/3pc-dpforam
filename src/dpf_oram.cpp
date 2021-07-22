#include "dpf_oram.h"

#include <assert.h>
#include <omp.h>

#include <iostream>

#include "util.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const char *party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint log_n, uint data_bytes, bool is_last) : Protocol(party, connections, rnd, prgs) {
    this->is_last_ = is_last;
    this->tau_ = is_last ? std::max(5 - (int)log2(data_bytes), 0) : tau;
    this->log_n_ = (log_n <= this->tau_ || !is_last) ? log_n : (log_n - this->tau_);
    this->ttp_ = 1 << this->tau_;
    this->log_n_bytes_ = (this->log_n_ + 7) / 8 + 1;
    this->next_log_n_ = is_last ? 0 : log_n + tau;
    this->next_log_n_bytes_ = is_last ? data_bytes : (this->next_log_n_ + 7) / 8 + 1;
    this->data_bytes_ = this->next_log_n_bytes_ * this->ttp_;
    this->n_ = 1ul << this->log_n_;
    this->is_first_ = this->log_n_ < 2 * tau;

    this->InitMem(this->rom_[0]);
    this->InitMem(this->rom_[1]);
    if (this->is_first_) {
        this->wom_ = NULL;
        this->position_map_ = NULL;
    } else {
        this->InitMem(this->wom_);
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
        this->DeleteMem(this->wom_);
    }
    this->DeleteMem(this->rom_[0]);
    this->DeleteMem(this->rom_[1]);
}

void DPFORAM::Init() {
    this->InitCacheCtr();
    this->SetMemZero(rom_[0]);
    this->SetMemZero(rom_[1]);
    this->SetMemZero(wom_);
    if (!is_first_) {
        this->position_map_->Init();
    }
}

void DPFORAM::InitCacheCtr() {
    this->read_cache_ctr_ = 1;
}

void DPFORAM::InitMem(uchar **&mem) {
    mem = new uchar *[this->n_];
    for (unsigned long i = 0; i < this->n_; i++) {
        mem[i] = new uchar[this->data_bytes_];
    }
}

void DPFORAM::DeleteMem(uchar **mem) {
    for (unsigned long i = 0; i < this->n_; i++) {
        delete[] mem[i];
    }
    delete[] mem;
}

void DPFORAM::SetMemZero(uchar **mem) {
    if (mem == NULL) return;
#pragma omp parallel for
    for (unsigned long i = 0; i < this->n_; i++) {
        memset(mem[i], 0, this->data_bytes_);
    }
}

// private
void DPFORAM::BlockPIR(const unsigned long addr_23[2],
                       const uchar *const *const mem_23[2], unsigned long size, uchar *block_23[2],
                       uchar *fss_out[2]) {
    uchar *keys[2];
    uint key_bytes = fss_.Gen(addr_23[0] ^ addr_23[1], this->log_n_, keys);
    this->connnections_[0]->Write(keys[0], key_bytes);
    this->connnections_[1]->Write(keys[1], key_bytes);
    this->connnections_[0]->Read(keys[1], key_bytes);
    this->connnections_[1]->Read(keys[0], key_bytes);

    uint quo = this->data_bytes_ / 16;
    uint rem = this->data_bytes_ % 16;
    memset(block_23[0], 0, this->data_bytes_);

    if (omp_get_max_threads() == 1) {
        for (uint i = 0; i < 2; i++) {
            this->fss_.EvalAllWithPerm(keys[i], log_n_, addr_23[i], fss_out[i]);
            for (unsigned long j = 0; j < size; j++) {
                // if (fss_out[i][j])
                {
                    // set_xor_128(mem_23[i][j], quo, rem, block_23[0]);
                    select_xor_128(mem_23[i][j], fss_out[i][j], quo, rem, block_23[0]);
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

            uchar tmp[data_bytes_];
            memset(tmp, 0, data_bytes_ * sizeof(uchar));
#pragma omp for collapse(2)
            for (uint i = 0; i < 2; i++) {
                for (unsigned long j = 0; j < size; j++) {
                    //					if (fss_out[i][j])
                    {
                        //						set_xor_128(mem_23[i][j], quo, rem, tmp);
                        select_xor_128(mem_23[i][j], fss_out[i][j], quo, rem, tmp);
                    }
                }
            }
#pragma omp critical
            {
                set_xor_128(tmp, quo, rem, block_23[0]);
            }
        }
    }

    connnections_[0]->Write(block_23[0], data_bytes_);
    connnections_[1]->Read(block_23[1], data_bytes_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::RecPIR(const uint idx_23[2], const uchar *const block_23[2],
                     uchar *rec_23[2]) {
    uchar *keys[2];
    uint keyBytes = fss_.Gen(idx_23[0] ^ idx_23[1], tau_, keys);
    connnections_[0]->Write(keys[0], keyBytes);
    connnections_[1]->Write(keys[1], keyBytes);
    connnections_[0]->Read(keys[1], keyBytes);
    connnections_[1]->Read(keys[0], keyBytes);

    memset(rec_23[0], 0, next_log_n_bytes_);
    for (uint i = 0; i < 2; i++) {
        uchar fss_out[ttp_];
        fss_.EvalAll(keys[i], tau_, fss_out);
        for (uint j = 0; j < ttp_; j++) {
            if (fss_out[j ^ idx_23[i]]) {
                cal_xor(rec_23[0], block_23[i] + j * next_log_n_bytes_,
                        next_log_n_bytes_, rec_23[0]);
            }
        }
    }

    connnections_[0]->Write(rec_23[0], next_log_n_bytes_);
    connnections_[1]->Read(rec_23[1], next_log_n_bytes_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::UpdateWOM(const uchar *const delta_block_23[2],
                        const uchar *const fss_out[2]) {
    uint quo = data_bytes_ / 16;
    uint rem = data_bytes_ % 16;
#pragma omp parallel for
    for (unsigned long j = 0; j < n_; j++) {
        for (uint i = 0; i < 2; i++) {
            //			if (fss_out[i][j])
            {
                //set_xor_128(delta_block_23[i], quo, rem, wom[j]);
                select_xor_128(delta_block_23[i], fss_out[i][j], quo, rem, wom_[j]);
            }
        }
    }
}

void DPFORAM::AppendCache(const uchar *const block_23[2],
                          const uchar *const delta_block_23[2]) {
    for (uint i = 0; i < 2; i++) {
        cal_xor(block_23[i], delta_block_23[i], data_bytes_, read_cache_[i][read_cache_ctr_]);
    }
    read_cache_ctr_++;
    if (read_cache_ctr_ == n_) {
        InitCacheCtr();
        WOM2ROM();
        position_map_->Init();
    }
}

// TODO: buffered read/write
void DPFORAM::WOM2ROM() {
    if (this->is_first_) {
        return;
    }
    for (unsigned long i = 0; i < n_; i++) {
        memcpy(this->rom_[0][i], this->wom_[i], this->data_bytes_);
    }
    for (unsigned long i = 0; i < n_; i++) {
        this->connnections_[0]->Write(this->wom_[i], this->data_bytes_);
        //		cons[0]->fwrite(wom[i], DBytes);
    }
    //	cons[0]->Flush();
    for (unsigned long i = 0; i < n_; i++) {
        this->connnections_[1]->Read(this->rom_[1][i], this->data_bytes_);
        //		cons[1]->fread(rom[1][i], DBytes);
    }
}

void DPFORAM::Access(const unsigned long addr_23[2], const uchar *const new_rec_23[2],
                     bool is_read, uchar *rec_23[2]) {
    uint mask = this->ttp_ - 1;
    unsigned long addr_pre_23[2];
    uint addr_suf_23[2];
    for (uint i = 0; i < 2; i++) {
        addr_pre_23[i] = addr_23[i] >> this->tau_;
        addr_suf_23[i] = (uint)addr_23[i] & mask;
    }

    if (is_first_) {
        uchar *block_23[2];
        uchar *fss_out[2];
        uchar *delta_rec_23[2];
        uchar *delta_block_23[2];
        uchar *delta_rom_23[2];
        for (uint i = 0; i < 2; i++) {
            block_23[i] = new uchar[this->data_bytes_];
            fss_out[i] = new uchar[this->n_];
            delta_rec_23[i] = new uchar[this->next_log_n_bytes_];
            delta_block_23[i] = new uchar[this->data_bytes_];
            delta_rom_23[i] = new uchar[this->n_ * this->data_bytes_];
        }
        BlockPIR(addr_pre_23, this->rom_, this->n_, block_23, fss_out);
        RecPIR(addr_suf_23, block_23, rec_23);

        for (uint i = 0; i < 2; i++) {
            if (is_read) {
                memset(delta_rec_23[i], 0, this->next_log_n_bytes_);
            } else {
                cal_xor(rec_23[i], new_rec_23[i], this->next_log_n_bytes_,
                        delta_rec_23[i]);
            }
        }
        // gen_delta_array(addr_suf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        uint int_addrPre_23[2] = {(uint)addr_pre_23[0], (uint)addr_pre_23[1]};
        // gen_delta_array(int_addrPre_23, (uint)N, DBytes, delta_block_23,
        //                 delta_rom_23);

        for (uint i = 0; i < 2; i++) {
            for (unsigned long j = 0; j < n_; j++) {
                cal_xor(rom_[i][j], delta_rom_23[i] + j * this->data_bytes_, this->data_bytes_,
                        rom_[i][j]);
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

    uchar new_stash_ptr[this->log_n_bytes_];
    long_to_bytes(this->read_cache_ctr_, new_stash_ptr, this->log_n_bytes_);
    new_stash_ptr[0] = 1;

    uchar *stash_ptr_23[2];
    uchar *new_stash_ptr_23[2];
    for (uint i = 0; i < 2; i++) {
        stash_ptr_23[i] = new uchar[this->log_n_bytes_];
        new_stash_ptr_23[i] = new_stash_ptr;
    }
    position_map_->Access(addr_pre_23, new_stash_ptr_23, false, stash_ptr_23);

    unsigned long mask2 = n_ - 1;
    unsigned long stash_addrPre_23[2];
    stash_addrPre_23[0] = bytes_to_long(stash_ptr_23[0], this->log_n_bytes_) & mask2;
    stash_addrPre_23[1] = bytes_to_long(stash_ptr_23[1], this->log_n_bytes_) & mask2;

    uchar *rom_block_23[2];
    uchar *stash_block_23[2];
    uchar *rom_fss_out[2];
    uchar *stash_fss_out[2];
    uchar *block_23[2];
    for (uint i = 0; i < 2; i++) {
        rom_block_23[i] = new uchar[this->data_bytes_];
        stash_block_23[i] = new uchar[this->data_bytes_];
        rom_fss_out[i] = new uchar[this->n_];
        stash_fss_out[i] = new uchar[this->n_];
        block_23[i] = new uchar[this->data_bytes_];
    }
    BlockPIR(addr_pre_23, rom_, n_, rom_block_23, rom_fss_out);
    BlockPIR(stash_addrPre_23, this->read_cache_, this->read_cache_ctr_, stash_block_23,
             stash_fss_out);

    uchar indicator_23[2] = {stash_ptr_23[0][0], stash_ptr_23[1][0]};
    // obliv_select(rom_block_23, stash_block_23, indicator_23, block_23);

    RecPIR(addr_suf_23, block_23, rec_23);
    uchar *delta_rec_23[2];
    uchar *delta_block_23[2];
    for (uint i = 0; i < 2; i++) {
        delta_rec_23[i] = new uchar[this->next_log_n_bytes_];
        if (is_read) {
            memset(delta_rec_23[i], 0, this->next_log_n_bytes_);
        } else {
            cal_xor(rec_23[i], new_rec_23[i], this->next_log_n_bytes_, delta_rec_23[i]);
        }
        delta_block_23[i] = new uchar[this->data_bytes_];
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
    std::cout << "2^tau: " << this->ttp_ << std::endl;
    std::cout << "logN: " << this->log_n_ << std::endl;
    std::cout << "N: " << this->n_ << std::endl;
    std::cout << "logNBytes: " << this->log_n_bytes_ << std::endl;
    std::cout << "next_log_n_: " << this->next_log_n_ << std::endl;
    std::cout << "nextLogNBytes: " << this->next_log_n_bytes_ << std::endl;
    std::cout << "DBytes: " << this->data_bytes_ << std::endl;
    std::cout << "Stash counter: " << this->read_cache_ctr_ << std::endl;
    std::cout << "ROM: " << (this->rom_ != NULL) << std::endl;
    std::cout << "WOM: " << (this->wom_ != NULL) << std::endl;
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
        rec_23[i] = new uchar[this->next_log_n_bytes_];
        new_rec_23[i] = new uchar[this->next_log_n_bytes_];
        memset(rec_23[i], 0, this->next_log_n_bytes_);
        memset(new_rec_23[i], 0, this->next_log_n_bytes_);
    }
    uchar rec_exp[this->next_log_n_bytes_];
    memset(rec_exp, 0, this->next_log_n_bytes_ * sizeof(uchar));
    if (strcmp(kParty, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        this->connnections_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(kParty, "debbie") == 0) {
        addr_23[1] = this->connnections_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(kParty, "eddie") == 0) {
            this->rnd_->GenerateBlock(new_rec_23[0], this->next_log_n_bytes_);
            this->connnections_[0]->Write(new_rec_23[0], this->next_log_n_bytes_, false);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[this->next_log_n_bytes_];
            connnections_[0]->Read(rec_out, this->next_log_n_bytes_);
            cal_xor(rec_out, rec_23[0], this->next_log_n_bytes_, rec_out);
            cal_xor(rec_out, rec_23[1], this->next_log_n_bytes_, rec_out);

            if (memcmp(rec_exp, rec_out, this->next_log_n_bytes_) == 0) {
                std::cout << "addr=" << addr_23[0] << ", t=" << t << ": Pass"
                          << std::endl;
            } else {
                std::cerr << "addr=" << addr_23[0] << ", t=" << t
                          << ": Fail !!!" << std::endl;
            }

            memcpy(rec_exp, new_rec_23[0], this->next_log_n_bytes_);
        } else if (strcmp(kParty, "debbie") == 0) {
            connnections_[1]->Read(new_rec_23[1], this->next_log_n_bytes_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            connnections_[1]->Write(rec_23[0], this->next_log_n_bytes_, false);
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
