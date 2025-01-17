#ifndef DPF_ORAM_H_
#define DPF_ORAM_H_

#include <assert.h>
#include <inttypes.h>
#include <omp.h>

#include <cmath>
#include <iostream>

#include "benchmark/constant.h"
#include "constant.h"
#include "data/binary_data.h"
#include "data/bulk_data.h"
#include "data/ec_data.h"
#include "data/zp_boost_data.h"
#include "data/zp_data.h"
#include "data/zp_debug_data.h"
#include "fss1bit.h"
#include "inv_gadget.h"
#include "peer.h"
#include "pir.h"
#include "piw.h"
#include "ssot.h"
#include "util.h"

template <typename K, typename D>
class ORAM {
private:
    uint party_;
    FSS1Bit fss_;
    Peer *peer_;
    uint n_;
    bool is_top_level_;

    std::vector<K> key_array_13_;
    std::vector<BulkData<D>> read_array_23_[2];
    std::vector<BulkData<D>> write_array_13_;
    std::vector<BulkData<D>> cache_array_23_[2];
    ORAM<BinaryData, BinaryData> *position_map_ = NULL;

    D last_read_data_13_;
    BulkData<D> last_read_block_13_;

private:
    void Init();
    void InitPositionMap();
    void InitArray(std::vector<BulkData<D>> &array, const uint n, const uint data_size);
    void ResetArray(std::vector<BulkData<D>> &array);

    BulkData<D> GetLatestData(BulkData<D> &read_block_13, BulkData<D> &cache_block_13, const bool is_cached_23[2], Benchmark::Record *benchmark);
    BulkData<D> DPFRead(const uint index_23[2], bool read_only, Benchmark::Record *benchmark);
    void DPFWrite(const uint index_23[2], BulkData<D> &old_block_13, BulkData<D> &new_block_13, Benchmark::Record *benchmark);
    void ReadPositionMap(const uint index_23[2], uint cache_index_23[2], bool is_cached[2], bool read_only, Benchmark::Record *benchmark);
    void AppendCache(BulkData<D> &new_block_13, Benchmark::Record *benchmark);
    void Flush(Benchmark::Record *benchmark);

public:
    ORAM(const uint party, Peer peer[2], uint n, uint key_size, uint data_size, bool is_top_level);
    ~ORAM();

    void SetKeyValueArray(std::vector<K> &key_array_13);
    void KeyToIndex(K key_23[2], uint index_23[2], Benchmark::Record *benchmark);

    D Read(const uint index_23[2], bool read_only, Benchmark::Record *benchmark);
    void Write(const uint index_23[2], D &new_data_13, Benchmark::Record *benchmark);

    void PrintMetadata();
    uint Size() {
        return this->n_;
    }
    uint DataSize() {
        return this->write_array_13_[0].Size() / DATA_PER_BLOCK;
    }
    void Reset();
    void Test(uint iterations);
};

#endif /* DPF_ORAM_H_ */
