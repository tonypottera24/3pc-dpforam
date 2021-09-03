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
    uchar **write_array_;
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
    void GetLatestData(const uchar *v_read, const uchar *v_cache, const bool is_used, uchar *v_old[2]);
    void ShareTwoThird(const uchar *v_in, const uint n, uchar *v_old[2]);
    void PIR(const uchar **array[2], const uint64_t n, const uint data_size, const uint64_t index[2], uchar *output);
    void PIW(uchar **array, const uint64_t n, const uint data_size, const uint64_t index[2], const uchar *data);
    void ReadPositionMap(const uint64_t index[2], uint64_t cache_index[2], bool *is_used);
    void Read(const uint64_t index[2], uchar *v_old[2]);
    void Write(const uint64_t index[2], const uchar *new_data);
    void AppendCache(const uchar *const block_23[2], const uchar *const delta_block_23[2]);
    void Flush();

public:
    DPFORAM(const uint party, Connection *cons[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs, uint tau,
            uint64_t n, uint data_size);
    ~DPFORAM();
    void Access(const uint64_t index[2], const uchar *new_data,
                const bool read_only, uchar *output[2]);
    void PrintMetadata();
    void Test(uint iter);
};

#endif /* DPF_ORAM_H_ */
