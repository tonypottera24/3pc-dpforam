#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <cmath>
#include <iostream>

#include "binary_data.h"
#include "fss1bit.h"
#include "inv_gadget.h"
#include "peer.h"
#include "pir.h"
#include "piw.h"
#include "ssot.h"
#include "util.h"
#include "zp_data.h"
template <typename D>
class DPFORAM {
private:
    uint party_;
    FSS1Bit fss_;
    Peer *peer_;

    std::vector<D> read_array_23_[2];
    std::vector<D> write_array_13_;
    std::vector<D> cache_array_23_[2];
    DPFORAM<BinaryData> *position_map_ = NULL;
    uint64_t tau_;
    uint64_t ssot_threshold_;
    uint64_t pseudo_dpf_threshold_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(std::vector<D> &array, const uint64_t n, const uint64_t data_size, bool set_zero);
    void ResetArray(std::vector<D> &array);
    void PrintArray(std::vector<D> &array, const char *array_name, const int64_t array_index = -1);

    D GetLatestData(D v_read_13,
                    D v_cache_13, const bool is_cached_23[2], bool count_band);

    D DPF_Read(const uint64_t index_23[2], bool read_only);
    D SSOT_Read(const uint64_t index_23[2], bool read_only);
    void DPF_Write(const uint64_t index_23[2], D old_data_13, D new_data_13, bool count_band);
    void ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached[2], bool read_only);
    void AppendCache(D v_new_13, bool count_band);
    void Flush(bool count_band);

public:
    DPFORAM(const uint party, Peer peer[2],
            uint64_t n, uint data_size, uint64_t tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold);
    ~DPFORAM();

    D Read(const uint64_t index_23[2], bool read_only);
    void Write(const uint64_t index_23[2], D old_data_13, D new_data_13, bool count_band);

    void PrintMetadata();
    uint64_t Size();
    uint DataSize();
    void Reset();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
