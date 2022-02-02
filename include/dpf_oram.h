#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <chrono>
#include <cmath>
#include <iostream>

#include "binary_data.h"
#include "ec_data.h"
#include "fss1bit.h"
#include "inv_gadget.h"
#include "peer.h"
#include "pir.h"
#include "piw.h"
#include "ssot.h"
#include "util.h"
#include "zp_data.h"
template <typename K, typename D>
class DPFORAM {
private:
    uint party_;
    FSS1Bit fss_;
    Peer *peer_;

    std::vector<K> key_array_13_;
    std::vector<D> read_array_23_[2];
    std::vector<D> write_array_13_;
    std::vector<D> cache_array_23_[2];
    DPFORAM<BinaryData, BinaryData> *position_map_ = NULL;
    uint tau_;
    uint ssot_threshold_;
    uint pseudo_dpf_threshold_;

    EVP_MD_CTX *md_ctx_ = EVP_MD_CTX_new();
    uchar *sha256_digest_ = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()));

private:
    void Init();
    void InitPositionMap();
    void InitArray(std::vector<D> &array, const uint n, const uint data_size);
    void ResetArray(std::vector<D> &array);
    void PrintArray(std::vector<D> &array, const char *array_name, const int64_t array_index = -1);

    D GetLatestData(D v_read_13,
                    D v_cache_13, const bool is_cached_23[2], bool count_band);
    D DPF_Read(const uint index_23[2], bool read_only);
    void DPF_Write(const uint index_23[2], D old_data_13, D new_data_13, bool count_band);
    void ReadPositionMap(const uint index_23[2], uint cache_index_23[2], bool is_cached[2], bool read_only);
    void AppendCache(D v_new_13, bool count_band);
    void Flush(bool count_band);

public:
    DPFORAM(const uint party, Peer peer[2],
            uint n, uint data_size, uint tau, uint ssot_threshold, uint pseudo_dpf_threshold);
    ~DPFORAM();

    void KeyToIndex(K key_23[2], uint index_23[2], bool count_band);

    D Read(const uint index_23[2], bool read_only);
    void Write(const uint index_23[2], D old_data_13, D new_data_13, bool count_band);

    void PrintMetadata();
    inline uint Size() {
        return this->write_array_13_.size();
    }
    inline uint DataSize() {
        return this->write_array_13_[0].Size();
    }
    void Reset();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
