#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

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
    uint tau_;
    uint data_size_;
    uint64_t n_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(uchar **&array);
    void DeleteArray(uchar **array);
    void GetLatestData(const uchar *v_read,
                       const uchar *v_cache, const uchar *v_meta,
                       uchar *v_out);
    void ShareTwoThird(const uchar *v_in, const uint n, uchar *v_old[2]);
    void PIR(const uchar **array[2], const uint64_t n, const uint data_size, const uint64_t index[2], uchar *output);
    void PIW(uchar **array, const uint64_t n, const uint data_size, const uint64_t index_23[2], const uchar *v_delta);
    void ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], uchar v_meta[1]);
    void Read(const uint64_t index[2], uchar *v_old[2]);
    void Write(const uint64_t index[2], const uchar *new_data);
    void AppendCache(const uchar *v_new);
    void Flush();
    void Reset();

public:
    DPFORAM(const uint party, Connection *connections[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
            uint64_t n, uint data_size, uint tau);
    ~DPFORAM();
    void Access(const uint64_t index[2], const uchar *new_data,
                const bool read_only, uchar *output[2]);
    void PrintMetadata();
    void Test(uint iter);
};

#endif /* DPF_ORAM_H_ */
