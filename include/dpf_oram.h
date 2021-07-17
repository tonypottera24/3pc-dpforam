#ifndef DPFORAM_H_
#define DPFORAM_H_

#include "fss1bit.h"
#include "protocol.h"

class DPFORAM : public Protocol {
private:
    static FSS1Bit fss_;
    uchar **rom_[2];
    uchar **wom_;
    uchar **stash_[2];
    DPFORAM *pos_map_;
    unsigned long stash_ctr_;

public:
    uint log_n_;
    uint log_n_bytes_;
    uint next_log_n_;
    uint next_log_n_bytes_;
    uint tau_;
    uint ttp_;
    uint d_bytes_;
    unsigned long n_;
    bool is_first_;
    bool is_last_;

private:
    void Init();
    void InitCtr();
    void SetZero(uchar **mem);
    void InitMem(uchar **&mem);
    void DeleteMem(uchar **mem);
    void BlockPIR(const unsigned long addr_23[2], const uchar *const *const mem_23[2],
                  unsigned long size, uchar *block_23[2], uchar *fss_out[2]);
    void RecPIR(const uint idx_23[2], const uchar *const block_23[2],
                uchar *rec_23[2]);
    void UpdateWOM(const uchar *const delta_block_23[2],
                   const uchar *const fss_out[2]);
    void AppendStash(const uchar *const block_23[2],
                     const uchar *const delta_block_23[2]);
    void WOM2ROM();

public:
    DPFORAM(const char *party, Connection *cons[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
            uint logN, uint DBytes, bool isLast);
    ~DPFORAM();
    void Access(const unsigned long addr_23[2], const uchar *const new_rec_23[2],
                bool isRead, uchar *rec_23[2]);
    void PrintMetadata();
    void Test(uint iter);
};

#endif /* DPFORAM_H_ */
