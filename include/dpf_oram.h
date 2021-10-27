#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <cmath>
#include <iostream>

#include "binary_data.h"
#include "connection.h"
#include "fss1bit.h"
#include "util.h"
#include "zp_data.h"

template <typename D>
class DPFORAM {
private:
    uint party_;
    FSS1Bit fss_;
    Connection **conn_;
    CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs_;

    std::vector<D> read_array_23_[2];
    std::vector<D> write_array_13_;
    std::vector<D> cache_array_23_[2];
    DPFORAM<BinaryData> *position_map_;
    uint64_t tau_;
    uint64_t ssot_threshold_;
    uint64_t pseudo_dpf_threshold_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(std::vector<D> &array, const uint64_t n, const uint64_t data_size, bool set_zero);
    void ResetArray(std::vector<D> &array);
    void PrintArray(std::vector<D> &array, const char *array_name, const int64_t array_index = -1);

    D *GetLatestData(D &v_read_13,
                     D &v_cache_13, const bool is_cached_23[2], bool count_band);

    D *DPF_Read(const uint64_t index_23[2], bool read_only);
    D *SSOT_Read(const uint64_t index_23[2], bool read_only);
    void DPF_Write(const uint64_t index_23[2], D old_data_23[2], D new_data_23[2], bool count_band);
    void ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached[2], bool read_only);
    void AppendCache(D v_new_23[2], bool count_band);
    void Flush(bool count_band);

    template <typename DD>
    DD *ShareTwoThird(DD &v_in_13, bool count_band);
    template <typename DD>
    std::vector<DD> *ShareTwoThird(std::vector<DD> &v_in_13, bool count_band);
    void ShareIndexTwoThird(const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band);

    template <typename DD>
    DD &PIR(std::vector<DD> array_23[2], const uint64_t index_23[2], bool count_band);
    template <typename DD>
    DD &DPF_PIR(std::vector<DD> array_23[2], const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], bool count_band);
    template <typename DD>
    DD &PSEUDO_DPF_PIR(std::vector<DD> array_23[2], const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], bool count_band);
    template <typename DD>
    DD &SSOT_PIR(std::vector<DD> &array_13, const uint64_t index_23[2], bool count_band);

    template <typename DD>
    void PIW(std::vector<DD> &array_13, const uint64_t index_23[2], DD v_delta_23[2], bool count_band);
    template <typename DD>
    void DPF_PIW(std::vector<DD> &array_13, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], DD v_delta_23[2], bool count_band);
    template <typename DD>
    void PSEUDO_DPF_PIW(std::vector<DD> &array_13, const uint64_t n, const uint64_t log_n, const uint64_t index_23[2], DD v_delta_23[2], bool count_band);

    void SSOT_P2(const uint64_t n, const uint64_t data_size, bool count_band);
    D *SSOT_P0(const uint64_t b0, std::vector<D> &u, bool count_band);
    D *SSOT_P1(const uint64_t b1, std::vector<D> &v, bool count_band);

public:
    DPFORAM(const uint party, Connection *connections[2],
            CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
            uint64_t n, uint64_t data_size, uint64_t tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold);
    ~DPFORAM();

    D *Read(const uint64_t index_23[2], bool read_only);
    void Write(const uint64_t index_23[2], D old_data_23[2], D new_data_23[2], bool count_band);

    void PrintMetadata();
    uint64_t Size();
    uint DataSize();
    void Reset();
    void Test(uint iterations);
};

#include "pir.h"
#include "piw.h"
#include "ssot.h"

#endif /* DPF_ORAM_H_ */
