#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <omp.h>

#include <iostream>

#include "fss1bit.h"
#include "protocol.h"
#include "ssot.h"
#include "util.h"

class DPFORAM : public Protocol {
private:
    static FSS1Bit fss_;
    uchar **read_array_[2];
    uchar **write_array_[2];
    uchar **cache_[2];
    DPFORAM *position_map_;
    uint64_t cache_ctr_;

public:
    uint tau_;
    uint data_size_;
    uint64_t n_;
    bool is_first_;
    bool is_last_;

private:
    void Init();
    void InitCacheCtr();
    void SetMemZero(uchar **mem);
    void InitArray(uchar **&array);
    void DeleteArray(uchar **array);
    void BlockPIR(const uint64_t addr_23[2], const uchar *const *const mem_23[2],
                  uint64_t size, uchar *block_23[2], uchar *fss_out[2]);
    void DataPIR(const uint idx_23[2], const uchar *const block_23[2],
                 uchar *rec_23[2]);
    void CheckPositionMap(const uchar *const rom_block_23[2],
                          const uchar *const stash_block_23[2], const uchar indicator_23[2],
                          uchar *block_23[2]);
    void PIR(const uchar **array[2], const uint64_t n, const uint data_size, const uint64_t index[2], uchar *output);
    void PIW(const uchar *const delta_block_23[2],
             const uchar *const fss_out[2]);
    void AppendCache(const uchar *const block_23[2],
                     const uchar *const delta_block_23[2]);
    void Flush();

public:
    DPFORAM(const uint party, Connection *cons[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
            uint64_t n, uint data_size);
    ~DPFORAM();
    void Access(const uint64_t index[2], const uchar *new_data,
                const bool read_only, uchar *output);
    void PrintMetadata();
    void Test(uint iter);
};

#endif /* DPF_ORAM_H_ */
