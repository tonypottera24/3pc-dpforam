#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <cmath>
#include <iostream>

#include "binary_data.h"
// #include "data.h"
#include "fss1bit.h"
#include "protocol.h"
#include "ssot.h"
#include "util.h"

class DPFORAM : public Protocol {
private:
    static FSS1Bit fss_;
    std::vector<Data> read_array_23_[2];
    std::vector<Data> write_array_13_;
    std::vector<Data> cache_array_23_[2];
    DPFORAM *position_map_;
    uint64_t tau_;
    uint64_t ssot_threshold_;
    uint64_t pseudo_dpf_threshold_;
    const DataType data_type_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(std::vector<Data> &array, const uint64_t n, const uint64_t data_size, bool set_zero = false);
    void ResetArray(std::vector<Data> &array);
    void PrintArray(std::vector<Data> &array, const char *array_name, const int64_t array_index = -1);

    Data *GetLatestData(Data &v_read_13,
                        Data &v_cache_13, const bool is_cached_23[2], bool count_band = true);
    Data *ShareTwoThird(Data &v_in_13, bool count_band = true);
    void ShareIndexTwoThird(const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band = true);

    Data &PIR(std::vector<Data> array[2], const uint64_t index[2], bool count_band = true);
    Data &DPF_PIR(std::vector<Data> array_23[2], const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], bool count_band = true);
    Data &PSEUDO_DPF_PIR(std::vector<Data> array_23[2], const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], bool count_band = true);
    Data &SSOT_PIR(std::vector<Data> &array_13, const uint64_t index_23[2], bool count_band = true);

    void PIW(std::vector<Data> &array, const uint64_t index_23[2], Data v_delta_23[2], bool count_band = true);
    void DPF_PIW(std::vector<Data> &array, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], Data v_delta_23[2], bool count_band = true);
    void PSEUDO_DPF_PIW(std::vector<Data> &array, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], Data v_delta_23[2], bool count_band = true);

    Data *DPF_Read(const uint64_t index_23[2], bool read_only = false);
    Data *SSOT_Read(const uint64_t index_23[2], bool read_only = false);

    void DPF_Write(const uint64_t index_23[2], Data old_data_23[2], Data new_data_23[2], bool count_band = true);

    void ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached[2], bool read_only = false);
    void AppendCache(Data v_new_23[2], bool count_band = true);
    void Flush(bool count_band = true);
    void Reset();

public:
    DPFORAM(const uint party, const DataType data_type, Connection *connections[2],
            CryptoPP::AutoSeededRandomPool *rnd,
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
            uint64_t n, uint64_t data_size, uint64_t tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold);
    ~DPFORAM();
    Data *Read(const uint64_t index_23[2], bool read_only = false);
    void Write(const uint64_t index_23[2], Data old_data_23[2], Data new_data_23[2], bool count_band = true);
    void PrintMetadata();
    uint64_t Size();
    uint DataSize();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
