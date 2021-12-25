#include "dpf_oram.h"

template <typename D, typename K>
DPFORAM<D, K>::DPFORAM(const uint party, Peer peer[2],
                       uint64_t n, uint data_size, uint tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold) : party_(party), peer_(peer), tau_(tau), ssot_threshold_(ssot_threshold), pseudo_dpf_threshold_(pseudo_dpf_threshold) {
    debug_print("DPFORAM n = %llu, data_size = %u, tau = %u\n", n, data_size, tau);
    this->InitArray(this->write_array_13_, n, data_size, true);

    if (n > 1ULL) {
        for (uint b = 0; b < 2; b++) {
            this->InitArray(this->read_array_23_[b], n, data_size, true);
        }

        uint64_t pos_data_per_block = 1 << this->tau_;
        uint64_t pos_n = uint64_ceil_divide(n, pos_data_per_block);
        uint64_t pos_data_size = byte_length(n << 1) * pos_data_per_block;
        this->position_map_ = new DPFORAM<BinaryData, BinaryData>(party, peer, pos_n, pos_data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    }
}

template <typename D, typename K>
DPFORAM<D, K>::~DPFORAM() {
    if (this->position_map_ != NULL) {
        delete this->position_map_;
    }
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b].clear();
        this->cache_array_23_[b].clear();
    }
    this->write_array_13_.clear();
}

template <typename D, typename K>
void DPFORAM<D, K>::Reset() {
    debug_print("[%llu]Reset\n", this->Size());
    for (uint b = 0; b < 2; b++) {
        ResetArray(read_array_23_[b]);
        cache_array_23_[b].clear();
    }
    ResetArray(write_array_13_);
    if (this->position_map_ != NULL) {
        this->position_map_->Reset();
    }
}

template <typename D, typename K>
void DPFORAM<D, K>::InitArray(std::vector<D> &array, const uint64_t n, const uint data_size, bool set_zero) {
    for (uint64_t i = 0; i < n; i++) {
        array.emplace_back(data_size, set_zero);
    }
}

template <typename D, typename K>
void DPFORAM<D, K>::ResetArray(std::vector<D> &array) {
    for (uint64_t i = 0; i < array.size(); i++) {
        array[i].Reset();
    }
}

template <typename D, typename K>
void DPFORAM<D, K>::PrintArray(std::vector<D> &array, const char *array_name, const int64_t array_index) {
    if (array_index == -1) {
        debug_print("%s:\n", array_name);
    } else {
        debug_print("%s[%llu]:\n", array_name, array_index);
    }
    for (uint64_t i = 0; i < array.size(); i++) {
        debug_print("[%llu] ", i);
        array[i].Print();
    }
    debug_print("\n");
}

// template <typename K>
// uint64_t DPFORAM<K>::KeyToIndex(K key, bool count_band) {
//     uint64_t index_13 = PIR::DPF_KEY_PIR<K>(this->peer_, this->fss_, this->key_array_13_, log_n, key, this->pseudo_dpf_threshold_, count_band);
//     return index_13;
// }

template <typename D, typename K>
D DPFORAM<D, K>::GetLatestData(D v_read_13,
                               D v_cache_13, const bool is_cached_23[2], bool count_band) {
    debug_print("[%llu]GetLatestData, is_cached_23 = (%u, %u)\n", this->Size(), is_cached_23[0], is_cached_23[1]);

    uint data_size = v_read_13.Size();
    D v_out_13;
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;

        this->peer_[P0].WriteData(v_read_13, count_band);
        this->peer_[P0].WriteData(v_cache_13, count_band);

        SSOT::P2<D>(this->peer_, 2, data_size, count_band);

        v_out_13.Random(this->peer_[P1].PRG(), data_size);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;
        D v_read_12 = this->peer_[P2].template ReadData<D>(data_size) + v_read_13;
        D v_cache_12 = this->peer_[P2].template ReadData<D>(data_size) + v_cache_13;

        std::vector<D> u = {v_read_12, v_cache_12};
        const uint64_t b0 = is_cached_23[P1] ^ is_cached_23[P2];
        v_out_13 = SSOT::P0(this->peer_, b0, u, count_band);
    } else if (this->party_ == 1) {
        const uint P2 = 1;
        std::vector<D> v = {v_read_13, v_cache_13};
        const uint64_t b1 = is_cached_23[P2];
        v_out_13 = SSOT::P1(this->peer_, b1, v, count_band);
        D tmp;
        tmp.Random(this->peer_[P2].PRG(), data_size);
        v_out_13 -= tmp;
    }
    return v_out_13;
}

template <typename D, typename K>
void DPFORAM<D, K>::ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached_23[2], bool read_only) {
    uint64_t n = this->Size();
    uint data_size = byte_length(n << 1ULL);
    uint64_t data_per_block = 1ULL << this->tau_;
    debug_print("[%llu]ReadPositionMap, n = %llu, index_23 = (%llu, %llu), data_size = %u, data_per_block = %llu, read_only = %d\n", this->Size(), n, index_23[0], index_23[1], data_size, data_per_block, read_only);

    this->position_map_->PrintMetadata();

    uint64_t block_index_23[2];
    uint64_t data_index_23[2];
    for (uint b = 0; b < 2; b++) {
        block_index_23[b] = index_23[b] / data_per_block;
        data_index_23[b] = index_23[b] % data_per_block;
    }

    // read block from array
    BinaryData old_block_13 = this->position_map_->Read(block_index_23, read_only);
    BinaryData *old_block_23 = ShareTwoThird<BinaryData>(this->peer_, old_block_13, !read_only);
    ;
    old_block_13.Print("old_block_13");

    // read data from block
    std::vector<BinaryData> old_data_array_23[2];
    for (uint b = 0; b < 2; b++) {
        uchar *old_block_data = old_block_23[b].Dump();
        for (uint i = 0; i < data_per_block; i++) {
            old_data_array_23[b].emplace_back(&old_block_data[i * data_size], data_size);
        }
    }
    BinaryData old_data_13 = PIR::PIR<BinaryData>(this->peer_, this->fss_, old_data_array_23, data_index_23, this->pseudo_dpf_threshold_, !read_only);
    old_data_13.Print("old_data_13");
    BinaryData *old_data_23 = ShareTwoThird<BinaryData>(this->peer_, old_data_13, !read_only);
    old_data_23[0].Print("old_data_23[0]");
    old_data_23[1].Print("old_data_23[1]");

    for (uint b = 0; b < 2; b++) {
        cache_index_23[b] = bytes_to_uint64(old_data_23[b].Dump(), old_data_23[b].Size());
        is_cached_23[b] = cache_index_23[b] & 1;
        cache_index_23[b] = (cache_index_23[b] >> 1) % n;
    }

    if (!read_only) {
        // TODO only 1 party need to write
        BinaryData new_block_13 = old_block_13;

        std::vector<BinaryData> data_array_13;
        uchar *new_block_data = new_block_13.Dump();
        for (uint i = 0; i < data_per_block; i++) {
            data_array_13.emplace_back(&new_block_data[i * data_size], data_size);
        }
        uchar new_cache_index_bytes[data_size];
        uint64_t new_cache_index = (this->cache_array_23_[0].size() << 1) + 1ULL;
        debug_print("new_cache_index = %llu\n", new_cache_index);

        uint64_to_bytes(new_cache_index, new_cache_index_bytes, data_size);
        BinaryData new_data_13 = BinaryData(new_cache_index_bytes, data_size);
        new_data_13.Print("new_data_13");

        BinaryData delta_data_13 = new_data_13 - old_data_13;
        delta_data_13.Print("delta_data_13");

        PIW::PIW(this->party_, this->peer_, this->fss_, data_array_13, data_index_23, delta_data_13, this->pseudo_dpf_threshold_, !read_only);
        for (uint i = 0; i < data_per_block; i++) {
            memcpy(&new_block_data[i * data_size], data_array_13[i].Dump(), data_size);
        }
        new_block_13.Load(new_block_data, data_per_block * data_size);
        new_block_13.Print("new_block_13");

        this->position_map_->Write(block_index_23, old_block_13, new_block_13, !read_only);

        // this->position_map_->PrintMetadata();
    }
    delete[] old_data_23;
    this->position_map_->PrintMetadata();
}

template <typename D, typename K>
D DPFORAM<D, K>::Read(const uint64_t index_23[2], bool read_only) {
    debug_print("[%llu]Read, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);
    uint64_t n = this->Size();
    if (n == 1) {
        return this->write_array_13_[0];
    } else if (n >= this->ssot_threshold_) {
        return PIR::SSOT_PIR(this->party_, this->peer_, this->write_array_13_, index_23, !read_only);
    } else {
        return DPF_Read(index_23, read_only);
    }
}

template <typename D, typename K>
D DPFORAM<D, K>::DPF_Read(const uint64_t index_23[2], bool read_only) {
    debug_print("[%llu]DPF_Read, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);

    D v_read_13 = PIR::PIR(this->peer_, this->fss_, this->read_array_23_, index_23, this->pseudo_dpf_threshold_, !read_only);
    v_read_13.Print("v_read_13");

    uint64_t cache_index_23[2];
    bool is_cached_23[2];
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, is_cached_23, read_only);
    }
    debug_print("cache_index_23 = (%llu, %llu), is_cached_23 = (%u, %u)\n", cache_index_23[0], cache_index_23[1], is_cached_23[0], is_cached_23[1]);

    if (this->cache_array_23_->size() == 0) {
        return v_read_13;
    }
    D v_cache_13 = PIR::PIR(this->peer_, this->fss_, this->cache_array_23_, cache_index_23, this->pseudo_dpf_threshold_, !read_only);
    v_cache_13.Print("v_cache_13");
    return GetLatestData(v_read_13, v_cache_13, is_cached_23, !read_only);
}

template <typename D, typename K>
void DPFORAM<D, K>::Write(const uint64_t index_23[2], D v_old_13, D v_new_13, bool count_band) {
    debug_print("[%llu]Write, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);
    uint64_t n = this->write_array_13_.size();
    if (n == 1) {
        this->write_array_13_[0] = v_new_13;
    } else {
        DPF_Write(index_23, v_old_13, v_new_13, count_band);
    }
}

template <typename D, typename K>
void DPFORAM<D, K>::DPF_Write(const uint64_t index_23[2], D v_old_13, D v_new_13, bool count_band) {
    debug_print("[%llu]DPF_Write, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);

    D v_delta_13 = v_new_13 - v_old_13;

    PIW::PIW(this->party_, this->peer_, this->fss_, this->write_array_13_, index_23, v_delta_13, this->pseudo_dpf_threshold_, count_band);
    this->AppendCache(v_new_13, count_band);
}

template <typename D, typename K>
void DPFORAM<D, K>::AppendCache(D v_new_13, bool count_band) {
    debug_print("[%llu]AppendCache\n", this->Size());
    D *v_new_23 = ShareTwoThird(this->peer_, v_new_13, count_band);
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].push_back(v_new_23[b]);
    }
    if (this->cache_array_23_[0].size() == this->read_array_23_[0].size()) {
        this->Flush(count_band);
        if (this->position_map_ != NULL) {
            this->position_map_->Reset();
        }
    }
    debug_print("[%llu]AppendCache GG\n", this->Size());
}

template <typename D, typename K>
void DPFORAM<D, K>::Flush(bool count_band) {
    debug_print("[%llu]Flush\n", this->Size());
    std::vector<D> *array_23 = ShareTwoThird(this->peer_, this->write_array_13_, count_band);
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b] = array_23[b];
    }
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].clear();
    }
}

template <typename D, typename K>
void DPFORAM<D, K>::PrintMetadata() {
    debug_print("========== PrintMetadata ==========\n");
    debug_print("party_: %u\n", this->party_);
    debug_print("tau_: %u\n", this->tau_);
    debug_print("n: %lu\n", this->write_array_13_.size());
    debug_print("data_size: %u\n", this->DataSize());
    debug_print("cache_size: %lu\n", this->cache_array_23_[0].size());
    debug_print("position_map_: %d\n", (this->position_map_ != NULL));

    this->PrintArray(this->read_array_23_[0], "read_array_23", 0);
    this->PrintArray(this->read_array_23_[1], "read_array_23", 1);
    this->PrintArray(this->write_array_13_, "write_array");
    this->PrintArray(this->cache_array_23_[0], "cache_23", 0);
    this->PrintArray(this->cache_array_23_[1], "cache_23", 1);

    debug_print("========== PrintMetadata ==========\n");
}

template <typename D, typename K>
void DPFORAM<D, K>::Test(uint iterations) {
    // TODO remember to free memory
    fprintf(stderr, "Test, iterations = %u \n", iterations);
    uint64_t party_time = 0;
    uint64_t time;

    for (uint iteration = 0; iteration < iterations; iteration++) {
        debug_print("Test, iteration = %u\n", iteration);
        uint64_t n = this->write_array_13_.size();
        // uint data_size = this->write_array_13_[0].Size();

        // this->InitArray(this->key_array_13_, n, data_size, false);

        uint64_t rand_range = n - 1;
        uint64_t index_13 = rand_uint64() % rand_range;
        uint64_t index_23[2];
        ShareIndexTwoThird<D>(this->peer_, index_13, n, index_23, false);
        // debug_print( "Test, rand_range = %llu, index_13 = %llu, index_23 = (%llu, %llu)\n", rand_range, index_13, index_23[0], index_23[1]);

        // this->PrintMetadata();

        debug_print("\nTest, ========== Read old data ==========\n");
        time = timestamp();
        D old_data_13 = this->Read(index_23, false);
        party_time += timestamp() - time;

        old_data_13.Print("old_data_13");

        this->PrintMetadata();

        debug_print("\nTest, ========== Write random data ==========\n");
        D new_data_13[3];
        new_data_13[2].Random(this->DataSize());
        new_data_13[2].Print("new_data_13[2]");

        time = timestamp();
        this->Write(index_23, old_data_13, new_data_13[2], true);
        party_time += timestamp() - time;

        this->PrintMetadata();

        debug_print("\nTest, ========== Read validation data ==========\n");
        D verify_data_13[3];
        verify_data_13[2] = this->Read(index_23, true);

        verify_data_13[2].Print("verify_data_13[2]");

        this->PrintMetadata();

        this->peer_[0].WriteData(verify_data_13[2], false);
        this->peer_[1].WriteData(verify_data_13[2], false);
        verify_data_13[0] = this->peer_[0].template ReadData<D>(this->DataSize());
        verify_data_13[1] = this->peer_[1].template ReadData<D>(this->DataSize());
        D verify_data = verify_data_13[0] + verify_data_13[1] + verify_data_13[2];
        verify_data.Print("verify_data");

        this->peer_[0].WriteData(new_data_13[2], false);
        this->peer_[1].WriteData(new_data_13[2], false);
        new_data_13[0] = this->peer_[0].template ReadData<D>(this->DataSize());
        new_data_13[1] = this->peer_[1].template ReadData<D>(this->DataSize());
        D new_data = new_data_13[0] + new_data_13[1] + new_data_13[2];
        new_data.Print("new_data");

        if (verify_data == new_data) {
            debug_print("%u, Pass\n", iteration);
        } else {
            fprintf(stderr, "%u, Fail !!!\n", iteration);
            exit(1);
        }
    }

    uint64_t party_bandwidth = this->peer_[0].Bandwidth() + this->peer_[1].Bandwidth();
    this->peer_[0].WriteLong(party_bandwidth, false);
    this->peer_[1].WriteLong(party_bandwidth, false);
    uint64_t total_bandwidth = party_bandwidth;
    total_bandwidth += this->peer_[0].ReadLong();
    total_bandwidth += this->peer_[1].ReadLong();

    this->peer_[0].WriteLong(party_time, false);
    this->peer_[1].WriteLong(party_time, false);
    uint64_t max_time = party_time;
    max_time = std::max(max_time, peer_[0].ReadLong());
    max_time = std::max(max_time, peer_[1].ReadLong());

    fprintf(stderr, "\n");
    fprintf(stderr, "n = %lu\n", this->write_array_13_.size());
    // fprintf(stderr, "Party Bandwidth(byte): %llu\n", party_bandwidth / iterations);
    // fprintf(stderr, "Party execution time(microsec): %llu\n", party_time / iterations);
    fprintf(stderr, "Total Bandwidth(byte): %llu\n", total_bandwidth / iterations);
    fprintf(stderr, "Max Execution time(microsec): %llu\n", max_time / iterations);
    fprintf(stderr, "\n");
}

template class DPFORAM<BinaryData, BinaryData>;
template class DPFORAM<BinaryData, ZpData>;
template class DPFORAM<ZpData, BinaryData>;
template class DPFORAM<ZpData, ZpData>;
