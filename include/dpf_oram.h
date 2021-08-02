#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include "fss1bit.h"
#include "protocol.h"

class DPFORAM : public Protocol {
private:
    static FSS1Bit fss_;
    uchar **read_array_[2];
    uchar **write_array_;
    uchar **read_cache_[2];
    DPFORAM *position_map_;
    uint64_t read_cache_ctr_;

public:
    uint pos_log_n_;
    uint pos_log_n_size_;
    uint oram_log_n_;
    uint oram_log_n_size_;
    uint tau_;
    uint data_per_block_;
    uint data_size_;
    uint64_t n_;
    bool is_first_;
    bool is_last_;

private:
    void Init();
    void InitCacheCtr();
    void SetMemZero(uchar **mem);
    void InitMem(uchar **&mem);
    void DeleteMem(uchar **mem);
    void BlockPIR(const uint64_t addr_23[2], const uchar *const *const mem_23[2],
                  uint64_t size, uchar *block_23[2], uchar *fss_out[2]);
    void DataPIR(const uint idx_23[2], const uchar *const block_23[2],
                 uchar *rec_23[2]);
    void UpdateWriteArray(const uchar *const delta_block_23[2],
                          const uchar *const fss_out[2]);
    void AppendCache(const uchar *const block_23[2],
                     const uchar *const delta_block_23[2]);
    void WOM2ROM();

public:
    DPFORAM(const char *party, Connection *cons[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
            uint log_n, uint d_bytes, bool is_last);
    ~DPFORAM();
    void Access(const uint64_t addr_23[2], const uchar *const new_rec_23[2],
                bool isRead, uchar *rec_23[2]);
    void PrintMetadata();
    void Test(uint iter);
};

#endif /* DPF_ORAM_H_ */
