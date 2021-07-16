#include "dpf_oram.h"

#include <assert.h>
#include <omp.h>

#include <iostream>

#include "util.h"

FSS1Bit DPFORAM::fss;

void DPFORAM::Init() {
    InitCtr();
    SetZero(rom[0]);
    SetZero(rom[1]);
    SetZero(wom);
    if (!isFirst) {
        pos_map->Init();
    }
}

void DPFORAM::InitCtr() {
    stash_ctr = 1;
}

void DPFORAM::SetZero(uchar **mem) {
    if (mem == NULL) {
        return;
    }
#pragma omp parallel for
    for (unsigned long i = 0; i < N; i++) {
        memset(mem[i], 0, DBytes);
    }
}

void DPFORAM::InitMem(uchar **&mem) {
    mem = new uchar *[N];
    for (unsigned long i = 0; i < N; i++) {
        mem[i] = new uchar[DBytes];
    }
}

void DPFORAM::DeleteMem(uchar **mem) {
    for (unsigned long i = 0; i < N; i++) {
        delete[] mem[i];
    }
    delete[] mem;
}

// private
void DPFORAM::BlockPIR(const unsigned long addr_23[2],
                       const uchar *const *const mem_23[2], unsigned long size, uchar *block_23[2],
                       uchar *fss_out[2]) {
    uchar *keys[2];
    uint keyBytes = fss.gen(addr_23[0] ^ addr_23[1], logN, keys);
    cons[0]->Write(keys[0], keyBytes);
    cons[1]->Write(keys[1], keyBytes);
    cons[0]->read(keys[1], keyBytes);
    cons[1]->read(keys[0], keyBytes);

    uint quo = DBytes / 16;
    uint rem = DBytes % 16;
    memset(block_23[0], 0, DBytes);

    if (omp_get_max_threads() == 1) {
        for (uint i = 0; i < 2; i++) {
            fss.eval_all_with_perm(keys[i], logN, addr_23[i], fss_out[i]);
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
                fss.eval_all_with_perm(keys[i], logN, addr_23[i], fss_out[i]);
            }

            uchar tmp[DBytes];
            memset(tmp, 0, DBytes * sizeof(uchar));
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

    cons[0]->Write(block_23[0], DBytes);
    cons[1]->read(block_23[1], DBytes);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::RecPIR(const uint idx_23[2], const uchar *const block_23[2],
                     uchar *rec_23[2]) {
    uchar *keys[2];
    uint keyBytes = fss.gen(idx_23[0] ^ idx_23[1], tau, keys);
    cons[0]->Write(keys[0], keyBytes);
    cons[1]->Write(keys[1], keyBytes);
    cons[0]->read(keys[1], keyBytes);
    cons[1]->read(keys[0], keyBytes);

    memset(rec_23[0], 0, nextLogNBytes);
    for (uint i = 0; i < 2; i++) {
        uchar fss_out[ttp];
        fss.eval_all(keys[i], tau, fss_out);
        for (uint j = 0; j < ttp; j++) {
            if (fss_out[j ^ idx_23[i]]) {
                cal_xor(rec_23[0], block_23[i] + j * nextLogNBytes,
                        nextLogNBytes, rec_23[0]);
            }
        }
    }

    cons[0]->Write(rec_23[0], nextLogNBytes);
    cons[1]->read(rec_23[1], nextLogNBytes);

    delete[] keys[0];
    delete[] keys[1];
}

void DPFORAM::UpdateWOM(const uchar *const delta_block_23[2],
                        const uchar *const fss_out[2]) {
    uint quo = DBytes / 16;
    uint rem = DBytes % 16;
#pragma omp parallel for
    for (unsigned long j = 0; j < N; j++) {
        for (uint i = 0; i < 2; i++) {
            //			if (fss_out[i][j])
            {
                //set_xor_128(delta_block_23[i], quo, rem, wom[j]);
                select_xor_128(delta_block_23[i], fss_out[i][j], quo, rem, wom[j]);
            }
        }
    }
}

void DPFORAM::AppendStash(const uchar *const block_23[2],
                          const uchar *const delta_block_23[2]) {
    for (uint i = 0; i < 2; i++) {
        cal_xor(block_23[i], delta_block_23[i], DBytes, stash[i][stash_ctr]);
    }
    stash_ctr++;
    if (stash_ctr == N) {
        InitCtr();
        WOM2ROM();
        pos_map->Init();
    }
}

// TODO: buffered read/write
void DPFORAM::WOM2ROM() {
    if (isFirst) {
        return;
    }
    for (unsigned long i = 0; i < N; i++) {
        memcpy(rom[0][i], wom[i], DBytes);
    }
    for (unsigned long i = 0; i < N; i++) {
        cons[0]->Write(wom[i], DBytes);
        //		cons[0]->fwrite(wom[i], DBytes);
    }
    //	cons[0]->flush();
    for (unsigned long i = 0; i < N; i++) {
        cons[1]->read(rom[1][i], DBytes);
        //		cons[1]->fread(rom[1][i], DBytes);
    }
}

DPFORAM::DPFORAM(const char *party, Connection *cons[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
                 uint logN, uint DBytes, bool isLast) : Protocol(party, cons, rnd, prgs) {
    this->isLast = isLast;
    this->tau = isLast ? std::max(5 - (int)log2(DBytes), 0) : tau;
    this->logN = (logN <= this->tau || !isLast) ? logN : (logN - this->tau);
    ttp = 1 << this->tau;
    logNBytes = (this->logN + 7) / 8 + 1;
    nextLogN = isLast ? 0 : logN + tau;
    nextLogNBytes = isLast ? DBytes : (nextLogN + 7) / 8 + 1;
    this->DBytes = nextLogNBytes * ttp;
    N = 1ul << this->logN;
    isFirst = this->logN < 2 * tau;

    InitMem(rom[0]);
    InitMem(rom[1]);
    if (isFirst) {
        wom = NULL;
        pos_map = NULL;
    } else {
        InitMem(wom);
        InitMem(stash[0]);
        InitMem(stash[1]);
        pos_map = new DPFORAM(party, cons, rnd, prgs, tau, this->logN - tau, 0,
                              false);
    }
    InitCtr();

    if (isLast) {
        Init();
    }
}

DPFORAM::~DPFORAM() {
    if (!isFirst) {
        delete pos_map;
        DeleteMem(stash[0]);
        DeleteMem(stash[1]);
        DeleteMem(wom);
    }
    DeleteMem(rom[0]);
    DeleteMem(rom[1]);
}

void DPFORAM::Access(const unsigned long addr_23[2], const uchar *const new_rec_23[2],
                     bool isRead, uchar *rec_23[2]) {
    uint mask = ttp - 1;
    unsigned long addrPre_23[2];
    uint addrSuf_23[2];
    for (uint i = 0; i < 2; i++) {
        addrPre_23[i] = addr_23[i] >> tau;
        addrSuf_23[i] = (uint)addr_23[i] & mask;
    }

    if (isFirst) {
        uchar *block_23[2];
        uchar *fss_out[2];
        uchar *delta_rec_23[2];
        uchar *delta_block_23[2];
        uchar *delta_rom_23[2];
        for (uint i = 0; i < 2; i++) {
            block_23[i] = new uchar[DBytes];
            fss_out[i] = new uchar[N];
            delta_rec_23[i] = new uchar[nextLogNBytes];
            delta_block_23[i] = new uchar[DBytes];
            delta_rom_23[i] = new uchar[N * DBytes];
        }
        BlockPIR(addrPre_23, rom, N, block_23, fss_out);
        RecPIR(addrSuf_23, block_23, rec_23);

        for (uint i = 0; i < 2; i++) {
            if (isRead) {
                memset(delta_rec_23[i], 0, nextLogNBytes);
            } else {
                cal_xor(rec_23[i], new_rec_23[i], nextLogNBytes,
                        delta_rec_23[i]);
            }
        }
        // gen_delta_array(addrSuf_23, ttp, nextLogNBytes, delta_rec_23,
        //                 delta_block_23);

        uint int_addrPre_23[2] = {(uint)addrPre_23[0], (uint)addrPre_23[1]};
        // gen_delta_array(int_addrPre_23, (uint)N, DBytes, delta_block_23,
        //                 delta_rom_23);

        for (uint i = 0; i < 2; i++) {
            for (unsigned long j = 0; j < N; j++) {
                cal_xor(rom[i][j], delta_rom_23[i] + j * DBytes, DBytes,
                        rom[i][j]);
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

    uchar new_stash_ptr[logNBytes];
    long_to_bytes(stash_ctr, new_stash_ptr, logNBytes);
    new_stash_ptr[0] = 1;

    uchar *stash_ptr_23[2];
    uchar *new_stash_ptr_23[2];
    for (uint i = 0; i < 2; i++) {
        stash_ptr_23[i] = new uchar[logNBytes];
        new_stash_ptr_23[i] = new_stash_ptr;
    }
    pos_map->Access(addrPre_23, new_stash_ptr_23, false, stash_ptr_23);

    unsigned long mask2 = N - 1;
    unsigned long stash_addrPre_23[2];
    stash_addrPre_23[0] = bytes_to_long(stash_ptr_23[0], logNBytes) & mask2;
    stash_addrPre_23[1] = bytes_to_long(stash_ptr_23[1], logNBytes) & mask2;

    uchar *rom_block_23[2];
    uchar *stash_block_23[2];
    uchar *rom_fss_out[2];
    uchar *stash_fss_out[2];
    uchar *block_23[2];
    for (uint i = 0; i < 2; i++) {
        rom_block_23[i] = new uchar[DBytes];
        stash_block_23[i] = new uchar[DBytes];
        rom_fss_out[i] = new uchar[N];
        stash_fss_out[i] = new uchar[N];
        block_23[i] = new uchar[DBytes];
    }
    BlockPIR(addrPre_23, rom, N, rom_block_23, rom_fss_out);
    BlockPIR(stash_addrPre_23, stash, stash_ctr, stash_block_23,
             stash_fss_out);

    uchar indicator_23[2] = {stash_ptr_23[0][0], stash_ptr_23[1][0]};
    // obliv_select(rom_block_23, stash_block_23, indicator_23, block_23);

    RecPIR(addrSuf_23, block_23, rec_23);
    uchar *delta_rec_23[2];
    uchar *delta_block_23[2];
    for (uint i = 0; i < 2; i++) {
        delta_rec_23[i] = new uchar[nextLogNBytes];
        if (isRead) {
            memset(delta_rec_23[i], 0, nextLogNBytes);
        } else {
            cal_xor(rec_23[i], new_rec_23[i], nextLogNBytes, delta_rec_23[i]);
        }
        delta_block_23[i] = new uchar[DBytes];
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
    std::cout << "Party: " << party << std::endl;
    std::cout << "Last level: " << isLast << std::endl;
    std::cout << "First level: " << isFirst << std::endl;
    std::cout << "tau: " << tau << std::endl;
    std::cout << "2^tau: " << ttp << std::endl;
    std::cout << "logN: " << logN << std::endl;
    std::cout << "N: " << N << std::endl;
    std::cout << "logNBytes: " << logNBytes << std::endl;
    std::cout << "nextLogN: " << nextLogN << std::endl;
    std::cout << "nextLogNBytes: " << nextLogNBytes << std::endl;
    std::cout << "DBytes: " << DBytes << std::endl;
    std::cout << "Stash counter: " << stash_ctr << std::endl;
    std::cout << "ROM: " << (rom != NULL) << std::endl;
    std::cout << "WOM: " << (wom != NULL) << std::endl;
    std::cout << "stash: " << (stash != NULL) << std::endl;
    std::cout << "posMap: " << (pos_map != NULL) << std::endl;
    std::cout << "===================\n"
              << std::endl;

    if (!isFirst) {
        pos_map->PrintMetadata();
    }
}

void DPFORAM::Test(uint iter) {
    unsigned long party_wc = 0;
    unsigned long wc;

    PrintMetadata();

    bool isRead = false;
    unsigned long range = 1ul << (logN + tau);
    unsigned long addr_23[2] = {10, 10};
    uchar *rec_23[2];
    uchar *new_rec_23[2];
    for (uint i = 0; i < 2; i++) {
        rec_23[i] = new uchar[nextLogNBytes];
        new_rec_23[i] = new uchar[nextLogNBytes];
        memset(rec_23[i], 0, nextLogNBytes);
        memset(new_rec_23[i], 0, nextLogNBytes);
    }
    uchar rec_exp[nextLogNBytes];
    memset(rec_exp, 0, nextLogNBytes * sizeof(uchar));
    if (strcmp(party, "eddie") == 0) {
        addr_23[0] = rand_long(range);
        cons[0]->WriteLong(addr_23[0], false);
    } else if (strcmp(party, "debbie") == 0) {
        addr_23[1] = cons[1]->ReadLong();
    }

    for (uint t = 0; t < iter; t++) {
        if (strcmp(party, "eddie") == 0) {
            rnd->GenerateBlock(new_rec_23[0], nextLogNBytes);
            cons[0]->Write(new_rec_23[0], nextLogNBytes, false);

            sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            uchar rec_out[nextLogNBytes];
            cons[0]->read(rec_out, nextLogNBytes);
            cal_xor(rec_out, rec_23[0], nextLogNBytes, rec_out);
            cal_xor(rec_out, rec_23[1], nextLogNBytes, rec_out);

            if (memcmp(rec_exp, rec_out, nextLogNBytes) == 0) {
                std::cout << "addr=" << addr_23[0] << ", t=" << t << ": Pass"
                          << std::endl;
            } else {
                std::cerr << "addr=" << addr_23[0] << ", t=" << t
                          << ": Fail !!!" << std::endl;
            }

            memcpy(rec_exp, new_rec_23[0], nextLogNBytes);
        } else if (strcmp(party, "debbie") == 0) {
            cons[1]->read(new_rec_23[1], nextLogNBytes);

            sync();
            wc = current_timestamp();
            Access(addr_23, new_rec_23, isRead, rec_23);
            party_wc += current_timestamp() - wc;

            cons[1]->Write(rec_23[0], nextLogNBytes, false);
        } else if (strcmp(party, "charlie") == 0) {
            sync();
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

    unsigned long party_band = bandwidth();
    cons[0]->WriteLong(party_band, false);
    cons[1]->WriteLong(party_band, false);
    unsigned long total_band = party_band;
    total_band += (unsigned long)cons[0]->ReadLong();
    total_band += (unsigned long)cons[1]->ReadLong();

    cons[0]->WriteLong(party_wc, false);
    cons[1]->WriteLong(party_wc, false);
    unsigned long max_wc = party_wc;
    max_wc = std::max(max_wc, (unsigned long)cons[0]->ReadLong());
    max_wc = std::max(max_wc, (unsigned long)cons[1]->ReadLong());

    std::cout << std::endl;
    std::cout << "Party Bandwidth(byte): " << (party_band / iter) << std::endl;
    std::cout << "Party Wallclock(microsec): " << (party_wc / iter)
              << std::endl;
    std::cout << "Total Bandwidth(byte): " << (total_band / iter) << std::endl;
    std::cout << "Max Wallclock(microsec): " << (max_wc / iter) << std::endl;
    std::cout << std::endl;
}
