#ifndef DPFORAM_H_
#define DPFORAM_H_

#include "fss.h"
#include "protocol.h"

class DPFORAM : public Protocol {
private:
    static FSS1Bit fss;
    uchar **rom[2];
    uchar **wom;
    uchar **stash[2];
    DPFORAM *pos_map;
    unsigned long stash_ctr;

public:
    uint logN;
    uint logNBytes;
    uint nextLogN;
    uint nextLogNBytes;
    uint tau;
    uint ttp;
    uint DBytes;
    unsigned long N;
    bool isFirst;
    bool isLast;

private:
    void init();
    void InitCtr();
    void set_zero(uchar **mem);
    void init_mem(uchar **&mem);
    void delete_mem(uchar **mem);
    void block_pir(const unsigned long addr_23[2], const uchar *const *const mem_23[2],
                   unsigned long size, uchar *block_23[2], uchar *fss_out[2]);
    void rec_pir(const uint idx_23[2], const uchar *const block_23[2],
                 uchar *rec_23[2]);
    void update_wom(const uchar *const delta_block_23[2],
                    const uchar *const fss_out[2]);
    void append_stash(const uchar *const block_23[2],
                      const uchar *const delta_block_23[2]);
    void wom_to_rom();

public:
    DPFORAM(const char *party, Connection *cons[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
            uint logN, uint DBytes, bool isLast);
    ~DPFORAM();
    void access(const unsigned long addr_23[2], const uchar *const new_rec_23[2],
                bool isRead, uchar *rec_23[2]);
    void print_metadata();
    void test(uint iter);
};

#endif /* DPFORAM_H_ */
