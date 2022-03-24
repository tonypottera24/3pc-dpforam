#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <chrono>
#include <cmath>
#include <iostream>

#include "binary_data.h"
#include "bulk_data.h"
#include "constant.h"
#include "ec_data.h"
#include "fss1bit.h"
#include "inv_gadget.h"
#include "peer.h"
#include "pir.h"
#include "piw.h"
#include "ssot.h"
#include "util.h"
#include "zp_data.h"
#include "zp_debug_data.h"

template <typename K, typename D>
class DPFORAM {
private:
    uint party_;
    FSS1Bit fss_;
    Peer *peer_;
    uint n_;

    std::vector<K> key_array_13_;
    std::vector<BulkData<D>> read_array_23_[2];
    std::vector<BulkData<D>> write_array_13_;
    std::vector<BulkData<D>> cache_array_23_[2];
    DPFORAM<BinaryData, BinaryData> *position_map_ = NULL;

    D last_read_data_13_;
    BulkData<D> last_read_block_13_;

    static inline EVP_MD_CTX *md_ctx_ = EVP_MD_CTX_new();
    static inline uchar *sha256_digest_ = (unsigned char *)OPENSSL_malloc(EVP_MD_size(EVP_sha256()));

private:
    void Init();
    void InitPositionMap();
    void InitArray(std::vector<BulkData<D>> &array, const uint n, const uint data_size);
    void ResetArray(std::vector<BulkData<D>> &array);

    BulkData<D> GetLatestData(BulkData<D> &read_block_13, BulkData<D> &cache_block_13, const bool is_cached_23[2], bool count_band);
    BulkData<D> DPF_Read(const uint index_23[2], bool read_only);
    void DPF_Write(const uint index_23[2], BulkData<D> &old_block_13, BulkData<D> &new_block_13, bool count_band);
    void ReadPositionMap(const uint index_23[2], uint cache_index_23[2], bool is_cached[2], bool read_only);
    void AppendCache(BulkData<D> &new_block_13, bool count_band);
    void Flush(bool count_band);

public:
    DPFORAM(const uint party, Peer peer[2], uint n, uint data_size);
    ~DPFORAM();

    void KeyToIndex(K key_23[2], uint index_23[2], bool count_band);

    D Read(const uint index_23[2], bool read_only);
    void Write(const uint index_23[2], D &new_data_13, bool count_band);

    void PrintMetadata();
    inline uint Size() {
        return this->n_;
    }
    inline uint DataSize() {
        return this->write_array_13_[0].Size() / DATA_PER_BLOCK;
    }
    void Reset();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
