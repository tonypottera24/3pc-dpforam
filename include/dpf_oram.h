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
    uint64_t tau_;
    uint64_t ssot_threshold_;
    uint64_t pseudo_dpf_threshold_;

public:
    uint64_t data_size_;
    uint64_t n_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(uchar **&array, bool set_zero = false);
    void ResetArray(uchar **array);
    void DeleteArray(uchar **array);
    void GetLatestData(uchar *v_read_13,
                       uchar *v_cache_13, const bool is_cached_23[2],
                       uchar *v_out_23[2], bool count_band = true);
    void ShareTwoThird(const uchar *v_in, const uint64_t n, uchar *v_old[2], bool count_band = true);
    void ShareIndexTwoThird(const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band = true);

    void PIR(uchar **array[2], const uint64_t n, const uint64_t data_size, const uint64_t index[2], uchar *v_out_13, bool count_band = true);
    void DPF_PIR(uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band = true);
    void PSEUDO_DPF_PIR(uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band = true);
    void SSOT_PIR(uchar **array_13, uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band = true);

    void PIW(uchar **array, const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_delta_23[2], bool count_band = true);
    void DPF_PIW(uchar **array, const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_delta_23[2], bool count_band = true);
    void PSEUDO_DPF_PIW(uchar **array, const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_delta_23[2], bool count_band = true);

    void DPF_Read(const uint64_t index_23[2], uchar *v_old_23[2], bool read_only = false);
    void SSOT_Read(const uint64_t index_23[2], uchar *v_out_23[2], bool read_only = false);

    void DPF_Write(const uint64_t index_23[2], uchar *old_data_23[2], uchar *new_data_23[2], bool count_band = true);
    void SSOT_Write(const uint64_t index_23[2], uchar *old_data_23[2], uchar *new_data_23[2], bool count_band = true);

    void ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached[2], bool read_only = false);
    void AppendCache(uchar *v_new_23[2], bool count_band = true);
    void Flush(bool count_band = true);
    void Reset();

public:
    DPFORAM(const uint party, Connection *connections[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
            uint64_t n, uint64_t data_size, uint64_t tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold);
    ~DPFORAM();
    void Read(const uint64_t index_23[2], uchar *v_old_23[2], bool read_only = false);
    void Write(const uint64_t index_23[2], uchar *old_data_23[2], uchar *new_data_23[2], bool count_band = true);
    void PrintMetadata();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
