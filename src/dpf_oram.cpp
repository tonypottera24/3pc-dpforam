#include "dpf_oram.h"

#include <assert.h>
#include <omp.h>

#include <iostream>

#include "util.h"

FSS1Bit DPFORAM::fss_;

void DPFORAM::Init() {
    InitCtr();
    SetZero(rom_[0]);
    SetZero(rom_[1]);
    SetZero(wom_);
    if (!is_first_) {
        pos_map_->Init();
    }
}

void DPFORAM::InitCtr() {
    stash_ctr_ = 1;
}

void DPFORAM::SetZero(uchar **mem) {
    if (mem == NULL) {
        return;
    }
#pragma omp parallel for
    for (unsigned long i = 0; i < n_; i++) {
        memset(mem[i], 0, d_bytes_);
    }
}

void DPFORAM::InitMem(uchar **&mem) {
    mem = new uchar *[n_];
    for (unsigned long i = 0; i < n_; i++) {
        mem[i] = new uchar[d_bytes_];
    }
}

void DPFORAM::DeleteMem(uchar **mem) {
    for (unsigned long i = 0; i < n_; i++) {
        delete[] mem[i];
    }
    delete[] mem;
}

// private
void DPFORAM::BlockPIR(const unsigned long addr_23[2],
                       const uchar *const *const mem_23[2], unsigned long size, uchar *block_23[2],
                       uchar *fss_out[2]) {
    uchar *keys[2];
    uint keyBytes = fss_.Gen(addr_23[0] ^ addr_23[1], log_n_, keys);
    cons_[0]->Write(keys[0], keyBytes);
    cons_[1]->Write(keys[1], keyBytes);
    cons_[0]->Read(keys[1], keyBytes);
    cons_[1]->Read(keys[0], keyBytes);

    uint quo = d_bytes_ / 16;
    uint rem = d_bytes_ % 16;
    memset(block_23[0], 0, d_bytes_);

    if (omp_get_max_threads() == 1) {
        for (uint i = 0; i < 2; i++) {
            fss_.EvalAllWithPerm(keys[i], log_n_, addr_23[i], fss_out[i]);
            for (unsigned long j = 0; j < size; j++) {
                //				if (fss_out[i][j])
                {
                    //					set_xor_128(mem_23[i][j], quo, rem, block_23[0]);
                    select_xor_128(mem_23[i][j], fss_out[i][j], quo, rem, block_23[0]);
                }
            }
        }
    } else {
#pragma omp parallel
        {
#pragma omp for
            for (uint i = 0; i < 2; i++) {
                fss_.EvalAllWithPerm(keys[i], log_n_, addr_23[i], fss_out[i]);
            }

            uchar tmp[d_bytes_];
            memset(tmp, 0, d_bytes_ * sizeof(uchar));
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

    cons_[0]->Write(block_23[0], d_bytes_);
    cons_[1]->Read(block_23[1], d_bytes_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::RecPIR(const uint idx_23[2], const uchar *const block_23[2],
                     uchar *rec_23[2]) {
    uchar *keys[2];
    uint keyBytes = fss_.Gen(idx_23[0] ^ idx_23[1], tau_, keys);
    cons_[0]->Write(keys[0], keyBytes);
    cons_[1]->Write(keys[1], keyBytes);
    cons_[0]->Read(keys[1], keyBytes);
    cons_[1]->Read(keys[0], keyBytes);

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

    cons_[0]->Write(rec_23[0], next_log_n_bytes_);
    cons_[1]->Read(rec_23[1], next_log_n_bytes_);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::UpdateWOM(const uchar *const delta_block_23[2],
                        const uchar *const fss_out[2]) {
    uint quo = d_bytes_ / 16;
    uint rem = d_bytes_ % 16;
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

void DPFORAM::AppendStash(const uchar *const block_23[2],
                          const uchar *const delta_block_23[2]) {
    for (uint i = 0; i < 2; i++) {
        cal_xor(block_23[i], delta_block_23[i], d_bytes_, stash_[i][stash_ctr_]);
    }
    stash_ctr_++;
    if (stash_ctr_ == n_) {
        InitCtr();
        WOM2ROM();
        pos_map_->Init();
    }
}

// TODO: buffered read/write
void DPFORAM::WOM2ROM() {
    if (is_first_) {
        return;
    }
    for (unsigned long i = 0; i < n_; i++) {
        memcpy(rom_[0][i], wom_[i], d_bytes_);
    }
    for (unsigned long i = 0; i < n_; i++) {
        cons_[0]->Write(wom_[i], d_bytes_);
        //		cons[0]->fwrite(wom[i], DBytes);
    }
    //	cons[0]->Flush();
    for (unsigned long i = 0; i < n_; i++) {
        cons_[1]->Read(rom_[1][i], d_bytes_);
        //		cons[1]->fread(rom[1][i], DBytes);
    }
}

DPFORAM::DPFORAM(const char *party, Connection *cons[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint logN, uint DBytes, bool isLast) : Protocol(party, cons, rnd, prgs) {
    this->is_last_ = isLast;
    this->tau_ = isLast ? std::max(5 - (int)log2(DBytes), 0) : tau;
    this->log_n_ = (logN <= this->tau_ || !isLast) ? logN : (logN - this->tau_);
    ttp_ = 1 << this->tau_;
    log_n_bytes_ = (this->log_n_ + 7) / 8 + 1;
    next_log_n_ = isLast ? 0 : logN + tau;
    next_log_n_bytes_ = isLast ? DBytes : (next_log_n_ + 7) / 8 + 1;
    this->d_bytes_ = next_log_n_bytes_ * ttp_;
    n_ = 1ul << this->log_n_;
    is_first_ = this->log_n_ < 2 * tau;

    InitMem(rom_[0]);
    InitMem(rom_[1]);
    if (is_first_) {
        wom_ = NULL;
        pos_map_ = NULL;
    } else {
        InitMem(wom_);
        InitMem(stash_[0]);
        InitMem(stash_[1]);
        pos_map_ = new DPFORAM(party, cons, rnd, prgs, tau, this->log_n_ - tau, 0,
                               false);
    }
    InitCtr();

    if (isLast) {
        Init();
    }
}

DPFORAM::~DPFORAM() {
    if (!is_first_) {
        delete pos_map_;
        DeleteMem(stash_[0]);
        DeleteMem(stash_[1]);
        DeleteMem(wom_);
    }
    DeleteMem(rom_[0]);
    DeleteMem(rom_[1]);
}

void DPFORAM::Access(const unsigned long addr_23[2], const uchar *const new_rec_23[2],
                     bool isRead, uchar *rec_23[2]) {
    uint mask = ttp_ - 1;
    unsigned long addrPre_23[2];
    uint addrSuf_23[2];
    for (uint i = 0; i < 2; i++) {
        addrPre_23[i] = addr_23[i] >> tau_;
        addrSuf_23[i] = (uint)addr_23[i] & mask;
    }

    if (is_first_) {
        uchar *block_23[2];
        uchar *fss_out[2];
        uchar *delta_rec_23[2];
        uchar *delta_block_23[2];
        uchar *delta_rom_23[2];
        for (uint i = 0; i < 2; i++) {
            block_23[i] = new uchar[d_bytes_];
            fss_out[i] = new uchar[n_];
            delta_rec_23[i] = new uchar[next_log_n_bytes_];
            delta_block_23[i] = new uchar[d_bytes_];
            delta_rom_23[i] = new uchar[n_ * d_bytes_];
        }
        BlockPIR(addrPre_23, rom_, n_, block_23, fss_out);
        RecPIR(addrSuf_23, block_23, rec_23);

        for (uint i = 0; i < 2; i++) {
            if (isRead) {
                memset(delta_rec_23[i], 0, next_log_n_bytes_);
            } else {
                cal_xor(rec_23[i], new_rec_23[i], next_log_n_bytes_,
                        delta_rec_23[i]);
            }
        }
        // gen_delta_array(addrSuf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        uint int_addrPre_23[2] = {(uint)addrPre_23[0], (uint)addrPre_23[1]};
        // gen_delta_array(int_addrPre_23, (uint)N, DBytes, delta_block_23,
        //                 delta_rom_23);

        for (uint i = 0; i < 2; i++) {
            for (unsigned long j = 0; j < n_; j++) {
                cal_xor(rom_[i][j], delta_rom_23[i] + j * d_bytes_, d_bytes_,
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

    uchar new_stash_ptr[log_n_bytes_];
    long_to_bytes(stash_ctr_, new_stash_ptr, log_n_bytes_);
    new_stash_ptr[0] = 1;

    uchar *stash_ptr_23[2];
    uchar *new_stash_ptr_23[2];
    for (uint i = 0; i < 2; i++) {
        stash_ptr_23[i] = new uchar[log_n_bytes_];
        new_stash_ptr_23[i] = new_stash_ptr;
    }
    pos_map_->Access(addrPre_23, new_stash_ptr_23, false, stash_ptr_23);

    unsigned long mask2 = n_ - 1;
    unsigned long stash_addrPre_23[2];
    stash_addrPre_23[0] = bytes_to_long(stash_ptr_23[0], log_n_bytes_) & mask2;
    stash_addrPre_23[1] = bytes_to_long(stash_ptr_23[1], log_n_bytes_) & mask2;

    uchar *rom_block_23[2];
    uchar *stash_block_23[2];
    uchar *rom_fss_out[2];
    uchar *stash_fss_out[2];
    uchar *block_23[2];
    for (uint i = 0; i < 2; i++) {
        rom_block_23[i] = new uchar[d_bytes_];
        stash_block_23[i] = new uchar[d_bytes_];
        rom_fss_out[i] = new uchar[n_];
        stash_fss_out[i] = new uchar[n_];
        block_23[i] = new uchar[d_bytes_];
    }
    BlockPIR(addrPre_23, rom_, n_, rom_block_23, rom_fss_out);
    BlockPIR(stash_addrPre_23, stash_, stash_ctr_, stash_block_23,
             stash_fss_out);

    uchar indicator_23[2] = {stash_ptr_23[0][0], stash_ptr_23[1][0]};
    // obliv_select(rom_block_23, stash_block_23, indicator_23, block_23);

    RecPIR(addrSuf_23, block_23, rec_23);
    uchar *delta_rec_23[2];
    uchar *delta_block_23[2];
    for (uint i = 0; i < 2; i++) {
        delta_rec_23[i] = new uchar[next_log_n_bytes_];
        if (isRead) {
            memset(delta_rec_23[i], 0, next_log_n_bytes_);
        } else {
            cal_xor(rec_23[i], new_rec_23[i], next_log_n_bytes_, delta_rec_23[i]);
        }
        delta_block_23[i] = new uchar[d_bytes_];
    }
    // gen_delta_array(addrSuf_23, ttp, nextLogNBytes, delta_rec_23,
    //                 delta_block_23);

    UpdateWOM(delta_block_23, rom_fss_out);
    AppendStash(block_23, delta_block_23);

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
    std::cout << "Last level: " << is_last_ << std::endl;
    std::cout << "First level: " << is_first_ << std::endl;
    std::cout << "tau: " << tau_ << std::endl;
    std::cout << "2^tau: " << ttp_ << std::endl;
    std::cout << "logN: " << log_n_ << std::endl;
    std::cout << "N: " << n_ << std::endl;
    std::cout << "logNBytes: " << log_n_bytes_ << std::endl;
    std::cout << "next_log_n_: " << next_log_n_ << std::endl;
    std::cout << "nextLogNBytes: " << next_log_n_bytes_ << std::endl;
    std::cout << "DBytes: " << d_bytes_ << std::endl;
    std::cout << "Stash counter: " << stash_ctr_ << std::endl;
    std::cout << "ROM: " << (rom_ != NULL) << std::endl;
    std::cout << "WOM: " << (wom_ != NULL) << std::endl;
    std::cout << "stash: " << (stash_ != NULL) << std::endl;
    std::cout << "posMap: " << (pos_map_ != NULL) << std::endl;
    std::cout << "===================\n"
              << std::endl;

    if (!is_first_) {
        pos_map_->PrintMetadata();
    }
}

void DPFORAM::Test(uint iter) {
    unsigned long party_wc = 0;
    unsigned long wc;

    PrintMetadata();

    bool isRead = false;
    unsigned long range = 1ul << (log_n_ + tau_);
    unsigned long addr_23[2] = {10, 10};
    uchar *rec_23[2];
    uchar *new_rec_23[2];
    for (uint i = 0; i < 2; i++) {
        rec_23[i] = new uchar[next_log_n_bytes_];
        new_rec_23[i] = new uchar[next_log_n_bytes_];
        memset(rec_23[i], 0, next_log_n_bytes_);
        memset(new_rec_23[i], 0, next_log_n_bytes_);
    }
    uchar rec_exp[next_log_n_bytes_];
    memset(rec_exp, 0, next_log_n_bytes_ * sizeof(uchar));
    if (strcmp(kParty, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        cons_[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(kParty, "debbie") == 0) {
        addr_23[1] = cons_[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(kParty, "eddie") == 0) {
            rnd_->GenerateBlock(new_rec_23[0], next_log_n_bytes_);
            cons_[0]->Write(new_rec_23[0], next_log_n_bytes_, false);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[next_log_n_bytes_];
            cons_[0]->Read(rec_out, next_log_n_bytes_);
            cal_xor(rec_out, rec_23[0], next_log_n_bytes_, rec_out);
            cal_xor(rec_out, rec_23[1], next_log_n_bytes_, rec_out);

            if (memcmp(rec_exp, rec_out, next_log_n_bytes_) == 0) {
                std::cout << "addr=" << addr_23[0] << ", t=" << t << ": Pass"
                          << std::endl;
            } else {
                std::cerr << "addr=" << addr_23[0] << ", t=" << t
                          << ": Fail !!!" << std::endl;
            }

            memcpy(rec_exp, new_rec_23[0], next_log_n_bytes_);
        } else if (strcmp(kParty, "debbie") == 0) {
            cons_[1]->Read(new_rec_23[1], next_log_n_bytes_);

            Sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            cons_[1]->Write(rec_23[0], next_log_n_bytes_, false);
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
    cons_[0]->WriteLong(party_band, false);
    cons_[1]->WriteLong(party_band, false);
    unsigned long total_band = party_band;
    total_band += (unsigned long)cons_[0]->ReadLong();
    total_band += (unsigned long)cons_[1]->ReadLong();

    cons_[0]->WriteLong(party_wc, false);
    cons_[1]->WriteLong(party_wc, false);
    unsigned long max_wc = party_wc;
    max_wc = std::max(max_wc, (unsigned long)cons_[0]->ReadLong());
    max_wc = std::max(max_wc, (unsigned long)cons_[1]->ReadLong());

    std::cout << std::endl;
    std::cout << "Party Bandwidth(byte): " << (party_band / iter) << std::endl;
    std::cout << "Party Wallclock(microsec): " << (party_wc / iter)
              << std::endl;
    std::cout << "Total Bandwidth(byte): " << (total_band / iter) << std::endl;
    std::cout << "Max Wallclock(microsec): " << (max_wc / iter) << std::endl;
    std::cout << std::endl;
}
