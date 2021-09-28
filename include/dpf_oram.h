#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <cmath>
#include <iostream>

#include "fss1bit.h"
#include "protocol.h"
#include "ssot.h"
#include "util.h"

class DPFORAM : public Protocol {
private:
    static FSS1Bit fss_;
    uchar **read_array_23_[2];
    uchar **write_array_13_;
    uchar **cache_23_[2];
    DPFORAM *position_map_;
    uint64_t cache_ctr_;

public:
    uint64_t tau_;
    uint64_t data_size_;
    uint64_t n_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(uchar **&array);
    void DeleteArray(uchar **array);
    void GetLatestData(const uchar *v_read_13,
                       const uchar *v_cache_13, const uchar *v_meta_13,
                       uchar *v_out_23[2], bool count_band = true);
    void ShareTwoThird(const uchar *v_in, const uint64_t n, uchar *v_old[2], bool count_band = true);
    void ShareIndexTwoThird(const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band = true);
    void PIR(uchar **array[2], const uint64_t n, const uint data_size, const uint64_t index[2], uchar *output, bool count_band = true);
    void PIW(uchar **array, const uint64_t n, const uint data_size, const uint64_t index_23[2], const uchar *v_delta_13, bool count_band = true);
    void ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], uchar v_meta[1], bool read_only = false);
    void AppendCache(const uchar *v_new, bool count_band = true);
    void Flush(bool count_band = true);
    void Reset();

public:
    DPFORAM(const uint party, Connection *connections[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
            uint64_t n, uint64_t data_size, uint64_t tau);
    ~DPFORAM();
    void Read(const uint64_t index[2], uchar *v_old[2], bool read_only = false);
    void Write(const uint64_t index_23[2], const uchar *old_data_13, const uchar *new_data_13, bool count_band = true);
    void PrintMetadata();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
