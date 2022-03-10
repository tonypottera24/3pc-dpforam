#include "dpf_oram.h"

template <typename K, typename D>
DPFORAM<K, D>::DPFORAM(const uint party, Peer peer[2],
                       uint n, uint data_size, uint tau, uint ssot_threshold, uint pseudo_dpf_threshold) : party_(party), peer_(peer), tau_(tau), ssot_threshold_(ssot_threshold), pseudo_dpf_threshold_(pseudo_dpf_threshold) {
    debug_print("DPFORAM n = %u, data_size = %u, tau = %u\n", n, data_size, tau);
    this->InitArray(this->write_array_13_, n, data_size);

    if (n > 1) {
        for (uint b = 0; b < 2; b++) {
            this->InitArray(this->read_array_23_[b], n, data_size);
        }

        uint pos_data_per_block = 1 << this->tau_;
        uint pos_n = divide_ceil(n, pos_data_per_block);
        uint pos_data_size = byte_length(n << 1) * pos_data_per_block;
        this->position_map_ = new DPFORAM<BinaryData, BinaryData>(party, peer, pos_n, pos_data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    }
}

template <typename K, typename D>
DPFORAM<K, D>::~DPFORAM() {
    if (this->position_map_ != NULL) {
        delete this->position_map_;
    }
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b].clear();
        this->cache_array_23_[b].clear();
    }
    this->write_array_13_.clear();
    EVP_MD_CTX_free(this->md_ctx_);
    OPENSSL_free(this->sha256_digest_);
}

template <typename K, typename D>
void DPFORAM<K, D>::Reset() {
    debug_print("[%u]Reset\n", this->Size());
    for (uint b = 0; b < 2; b++) {
        ResetArray(read_array_23_[b]);
        cache_array_23_[b].clear();
    }
    ResetArray(write_array_13_);
    if (this->position_map_ != NULL) {
        this->position_map_->Reset();
    }
}

template <typename K, typename D>
void DPFORAM<K, D>::InitArray(std::vector<D> &array, const uint n, const uint data_size) {
    array.resize(n, D(data_size));
}

template <typename K, typename D>
void DPFORAM<K, D>::ResetArray(std::vector<D> &array) {
    for (uint i = 0; i < array.size(); i++) {
        array[i].Reset();
    }
}

template <typename K, typename D>
void DPFORAM<K, D>::KeyToIndex(K key_23[2], uint index_23[2], bool count_band) {
    debug_print("[%u]KeyToIndex\n", this->Size());
    // bool pseudo = (this->Size() <= (1 << this->pseudo_dpf_threshold_));
    uint index_13 = PIR::DPF_KEY_PIR<K>(this->party_, this->peer_, this->fss_, this->key_array_13_, key_23, this->Size(), this->md_ctx_, this->sha256_digest_, count_band);
    ShareIndexTwoThird<K>(this->peer_, index_13, this->Size(), index_23, count_band);
    debug_print("[%u]KeyToIndex index_13 = %u, index_23 = (%u, %u)\n", this->Size(), index_13, index_23[0], index_23[1]);
}

template <typename K, typename D>
D DPFORAM<K, D>::GetLatestData(D v_read_13,
                               D v_cache_13, const bool is_cached_23[2], bool count_band) {
    debug_print("[%u]GetLatestData, is_cached_23 = (%u, %u)\n", this->Size(), is_cached_23[0], is_cached_23[1]);

    uint data_size = v_read_13.Size();
    D v_out_13(data_size);
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;

        this->peer_[P0].WriteData(v_read_13, count_band);
        this->peer_[P0].WriteData(v_cache_13, count_band);

        SSOT::P2<D>(this->peer_, 2, data_size, count_band);

        v_out_13.Random(this->peer_[P1].PRG());
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;
        D v_read_12 = this->peer_[P2].template ReadData<D>(data_size) + v_read_13;
        D v_cache_12 = this->peer_[P2].template ReadData<D>(data_size) + v_cache_13;

        std::vector<D> u = {v_read_12, v_cache_12};
        const uint b0 = is_cached_23[P1] ^ is_cached_23[P2];
        v_out_13 = SSOT::P0(this->peer_, b0, u, count_band);
    } else if (this->party_ == 1) {
        const uint P2 = 1;
        std::vector<D> v = {v_read_13, v_cache_13};
        const uint b1 = is_cached_23[P2];
        v_out_13 = SSOT::P1(this->peer_, b1, v, count_band);
        D tmp(data_size);
        tmp.Random(this->peer_[P2].PRG());
        v_out_13 -= tmp;
    }
    return v_out_13;
}

template <typename K, typename D>
void DPFORAM<K, D>::ReadPositionMap(const uint index_23[2], uint cache_index_23[2], bool is_cached_23[2], bool read_only) {
    uint n = this->Size();
    uint data_size = byte_length(n << 1);
    uint data_per_block = 1 << this->tau_;
    debug_print("[%u]ReadPositionMap, n = %u, index_23 = (%u, %u), data_size = %u, data_per_block = %u, read_only = %d\n", this->Size(), n, index_23[0], index_23[1], data_size, data_per_block, read_only);

    // this->position_map_->PrintMetadata();

    uint block_index_23[2];
    uint data_index_23[2];
    for (uint b = 0; b < 2; b++) {
        block_index_23[b] = index_23[b] / data_per_block;
        data_index_23[b] = index_23[b] % data_per_block;
    }

    // read block from array
    BinaryData old_block_13 = this->position_map_->Read(block_index_23, read_only);
    std::vector<BinaryData> old_block_23 = ShareTwoThird<BinaryData>(this->peer_, old_block_13, !read_only);
    old_block_13.Print("old_block_13");

    // read data from block
    std::vector<BinaryData> old_data_array_23[2];
    for (uint b = 0; b < 2; b++) {
        std::vector<uchar> dump = old_block_23[b].Dump();
        old_data_array_23[b].resize(data_per_block);
        for (uint i = 0; i < data_per_block; i++) {
            std::vector<uchar> data(dump.begin() + data_size * i, dump.begin() + data_size * (i + 1));
            old_data_array_23[b][i].Load(data);
        }
    }
    BinaryData old_data_13 = PIR::PIR<BinaryData>(this->peer_, this->fss_, old_data_array_23, data_index_23, this->pseudo_dpf_threshold_, !read_only);
    old_data_13.Print("old_data_13");
    std::vector<BinaryData> old_data_23 = ShareTwoThird<BinaryData>(this->peer_, old_data_13, !read_only);
    old_data_23[0].Print("old_data_23[0]");
    old_data_23[1].Print("old_data_23[1]");

    for (uint b = 0; b < 2; b++) {
        std::vector<uchar> dump = old_data_23[b].Dump();
        cache_index_23[b] = bytes_to_uint(dump.data(), dump.size());
        is_cached_23[b] = cache_index_23[b] & 1;
        cache_index_23[b] = (cache_index_23[b] >> 1) % n;
    }

    if (!read_only) {
        // TODO only 1 party need to write
        BinaryData new_block_13 = old_block_13;

        std::vector<BinaryData> data_array_13(data_per_block);

        std::vector<uchar> dump = new_block_13.Dump();
        for (uint i = 0; i < data_per_block; i++) {
            std::vector<uchar> data(dump.begin() + data_size * i, dump.begin() + data_size * (i + 1));
            data_array_13[i].Load(data);
        }
        uint new_cache_index = (this->cache_array_23_[0].size() << 1) + 1;
        debug_print("new_cache_index = %u\n", new_cache_index);

        std::vector<uchar> new_cache_index_bytes(data_size);
        uint_to_bytes(new_cache_index, new_cache_index_bytes.data(), data_size);
        BinaryData new_data_13;
        new_data_13.Load(new_cache_index_bytes);
        new_data_13.Print("new_data_13");

        BinaryData delta_data_13 = new_data_13 - old_data_13;
        delta_data_13.Print("delta_data_13");

        PIW::PIW(this->party_, this->peer_, this->fss_, data_array_13, data_index_23, delta_data_13, this->pseudo_dpf_threshold_, !read_only);
        std::vector<uchar> new_block_data;
        for (uint i = 0; i < data_array_13.size(); i++) {
            std::vector<uchar> dump = data_array_13[i].Dump();
            new_block_data.insert(new_block_data.end(), dump.begin(), dump.end());
        }
        new_block_13.Load(new_block_data);
        new_block_13.Print("new_block_13");

        this->position_map_->Write(block_index_23, old_block_13, new_block_13, !read_only);

        // this->position_map_->PrintMetadata();
    }
    // this->position_map_->PrintMetadata();
}

template <typename K, typename D>
D DPFORAM<K, D>::Read(const uint index_23[2], bool read_only) {
    debug_print("[%u]Read, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);
    uint n = this->Size();
    if (n == 1) {
        return this->write_array_13_[0];
    } else if (n <= this->ssot_threshold_) {
        return PIR::SSOT_PIR(this->party_, this->peer_, this->write_array_13_, index_23, !read_only);
    } else {
        return DPF_Read(index_23, read_only);
    }
}

template <typename K, typename D>
D DPFORAM<K, D>::DPF_Read(const uint index_23[2], bool read_only) {
    debug_print("[%u]DPF_Read, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);

    D v_read_13 = PIR::PIR(this->peer_, this->fss_, this->read_array_23_, index_23, this->pseudo_dpf_threshold_, !read_only);
    v_read_13.Print("v_read_13");

    uint cache_index_23[2];
    bool is_cached_23[2];
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, is_cached_23, read_only);
    }
    debug_print("cache_index_23 = (%u, %u), is_cached_23 = (%u, %u)\n", cache_index_23[0], cache_index_23[1], is_cached_23[0], is_cached_23[1]);

    if (this->cache_array_23_->size() == 0) {
        return v_read_13;
    }
    D v_cache_13 = PIR::PIR(this->peer_, this->fss_, this->cache_array_23_, cache_index_23, this->pseudo_dpf_threshold_, !read_only);
    v_cache_13.Print("v_cache_13");
    return GetLatestData(v_read_13, v_cache_13, is_cached_23, !read_only);
}

template <typename K, typename D>
void DPFORAM<K, D>::Write(const uint index_23[2], D v_old_13, D v_new_13, bool count_band) {
    debug_print("[%u]Write, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);
    uint n = this->write_array_13_.size();
    if (n == 1) {
        this->write_array_13_[0] = v_new_13;
    } else {
        DPF_Write(index_23, v_old_13, v_new_13, count_band);
    }
}

template <typename K, typename D>
void DPFORAM<K, D>::DPF_Write(const uint index_23[2], D v_old_13, D v_new_13, bool count_band) {
    debug_print("[%u]DPF_Write, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);

    D v_delta_13 = v_new_13 - v_old_13;

    PIW::PIW(this->party_, this->peer_, this->fss_, this->write_array_13_, index_23, v_delta_13, this->pseudo_dpf_threshold_, count_band);
    this->AppendCache(v_new_13, count_band);
}

template <typename K, typename D>
void DPFORAM<K, D>::AppendCache(D v_new_13, bool count_band) {
    debug_print("[%u]AppendCache\n", this->Size());
    std::vector<D> v_new_23 = ShareTwoThird(this->peer_, v_new_13, count_band);
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].push_back(v_new_23[b]);
    }
    if (this->cache_array_23_[0].size() == this->read_array_23_[0].size()) {
        this->Flush(count_band);
        if (this->position_map_ != NULL) {
            this->position_map_->Reset();
        }
    }
    debug_print("[%u]AppendCache GG\n", this->Size());
}

template <typename K, typename D>
void DPFORAM<K, D>::Flush(bool count_band) {
    debug_print("[%u]Flush\n", this->Size());
    std::vector<std::vector<D>> array_23 = ShareTwoThird(this->peer_, this->write_array_13_, count_band);
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b] = array_23[b];
    }
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].clear();
    }
}

template <typename K, typename D>
void DPFORAM<K, D>::PrintMetadata() {
#ifdef DEBUG
    debug_print("========== PrintMetadata ==========\n");
    debug_print("party_: %u\n", this->party_);
    debug_print("tau_: %u\n", this->tau_);
    debug_print("n: %lu\n", this->write_array_13_.size());
    debug_print("data_size: %u\n", this->DataSize());
    debug_print("cache_size: %lu\n", this->cache_array_23_[0].size());
    debug_print("position_map_: %d\n", (this->position_map_ != NULL));

    print_array(this->key_array_13_, "key_array");
    print_array(this->read_array_23_[0], "read_array_23", 0);
    print_array(this->read_array_23_[1], "read_array_23", 1);
    print_array(this->write_array_13_, "write_array");
    print_array(this->cache_array_23_[0], "cache_23", 0);
    print_array(this->cache_array_23_[1], "cache_23", 1);

    debug_print("========== PrintMetadata ==========\n");
#endif
}

template <typename K, typename D>
void DPFORAM<K, D>::Test(uint iterations) {
    // TODO remember to free memory
    fprintf(stderr, "Test, iterations = %u \n", iterations);

    bool key_value = !std::is_same<K, BinaryData>::value;

    uint64_t party_time = 0;
    std::chrono::high_resolution_clock::time_point t1, t2;
    uint n = this->Size();

    // socket seems need extra setup at first read/write
    this->peer_[0].WriteUInt(0, false);
    this->peer_[1].WriteUInt(0, false);
    this->peer_[0].ReadUInt();
    this->peer_[1].ReadUInt();

    if (key_value) {
        uint key_size = K().Size();
        std::vector<K> key_array_33[3];
        key_array_33[2].resize(n, K(key_size));
        for (uint i = 0; i < n; i++) {
            key_array_33[2][i].Random();
        }

        key_array_33[0] = write_read_data(this->peer_[0], key_array_33[2], this->peer_[1], n, key_size, false);
        key_array_33[1] = write_read_data(this->peer_[1], key_array_33[2], this->peer_[0], n, key_size, false);

        for (uint i = 0; i < n; i++) {
            K k = key_array_33[0][i] + key_array_33[1][i] + key_array_33[2][i];
            this->key_array_13_.push_back(k);
        }
    }

    for (uint iteration = 0; iteration < iterations; iteration++) {
        // fprintf(stderr, "Test, iteration = %u\n", iteration);

        uint index_23[2];
        uint key_size = K().Size();
        K key_23[2] = {K(key_size), K(key_size)};

        if (key_value) {
            // key_23[0].Random(this->peer_[0].PRG());
            // key_23[1].Random(this->peer_[1].PRG());
            uint index_33[3];
            index_33[2] = rand_uint() % n;
            this->peer_[0].WriteUInt(index_33[2], false);
            this->peer_[1].WriteUInt(index_33[2], false);
            index_33[0] = this->peer_[0].ReadUInt();
            index_33[1] = this->peer_[1].ReadUInt();
            uint index = index_33[0] ^ index_33[1] ^ index_33[2];
            if (this->party_ == 2) {
                key_23[0].Random(this->peer_[0].PRG());
                key_23[1].Random(this->peer_[1].PRG());
            } else {
                key_23[this->party_].Random(this->peer_[this->party_].PRG());
                this->peer_[1 - this->party_].WriteData(key_23[this->party_], false);
                K k = this->peer_[1 - this->party_].template ReadData<K>(key_size);
                key_23[1 - this->party_] = this->key_array_13_[index] - key_23[this->party_] - k;
            }

            t1 = std::chrono::high_resolution_clock::now();
            KeyToIndex(key_23, index_23, true);
            t2 = std::chrono::high_resolution_clock::now();
            party_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        } else {
            index_23[0] = rand_uint(this->peer_[0].PRG()) % n;
            index_23[1] = rand_uint(this->peer_[1].PRG()) % n;
            // debug_print( "Test, rand_range = %llu, index_13 = %llu, index_23 = (%llu, %llu)\n", rand_range, index_13, index_23[0], index_23[1]);
        }

        // this->PrintMetadata();

        this->peer_[0].WriteUInt(0, false);
        this->peer_[1].WriteUInt(0, false);
        this->peer_[0].ReadUInt();
        this->peer_[1].ReadUInt();

        // fprintf(stderr, "\nTest, ========== Read old data  ==========\n");
        debug_print("\nTest, ========== Read old data ==========\n");
        t1 = std::chrono::high_resolution_clock::now();
        D old_data_13 = this->Read(index_23, false);
        t2 = std::chrono::high_resolution_clock::now();
        uint64_t delta_time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        party_time += delta_time;

        old_data_13.Print("old_data_13");

        this->PrintMetadata();

        this->peer_[0].WriteUInt(0, false);
        this->peer_[1].WriteUInt(0, false);
        this->peer_[0].ReadUInt();
        this->peer_[1].ReadUInt();

        // fprintf(stderr, "\nTest, ========== Write random data ==========\n");
        debug_print("\nTest, ========== Write random data ==========\n");
        D new_data_13[3];
        new_data_13[2].Resize(this->DataSize());
        new_data_13[2].Random();
        new_data_13[2].Print("new_data_13[2]");

        t1 = std::chrono::high_resolution_clock::now();
        this->Write(index_23, old_data_13, new_data_13[2], true);
        t2 = std::chrono::high_resolution_clock::now();
        party_time += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        // printf("Write random data party_time = %llu\n", party_time);

        this->PrintMetadata();

        // fprintf(stderr, "\nTest, ========== Read validation data  ==========\n");
        debug_print("\nTest, ========== Read validation data ==========\n");
        if (key_value) {
            KeyToIndex(key_23, index_23, false);
        }

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

    uint party_bandwidth = this->peer_[0].Bandwidth() + this->peer_[1].Bandwidth();
    this->peer_[0].WriteUInt(party_bandwidth, false);
    this->peer_[1].WriteUInt(party_bandwidth, false);
    uint total_bandwidth = party_bandwidth;
    total_bandwidth += this->peer_[0].ReadUInt();
    total_bandwidth += this->peer_[1].ReadUInt();

    this->peer_[0].WriteLong(party_time, false);
    this->peer_[1].WriteLong(party_time, false);
    uint64_t max_time = party_time;
    max_time = std::max(max_time, peer_[0].ReadLong());
    max_time = std::max(max_time, peer_[1].ReadLong());

    fprintf(stderr, "\n");
    fprintf(stderr, "n = %lu\n", this->write_array_13_.size());
    // fprintf(stderr, "Party Bandwidth(byte): %llu\n", party_bandwidth / iterations);
    // fprintf(stderr, "Party execution time(microsec): %llu\n", party_time / iterations);
    fprintf(stderr, "Total Bandwidth(byte): %u\n", total_bandwidth / iterations);
    fprintf(stderr, "Max Execution time(microsec): %llu\n", max_time / iterations);
    fprintf(stderr, "\n");
}

template class DPFORAM<BinaryData, BinaryData>;
template class DPFORAM<BinaryData, ECData>;
template class DPFORAM<BinaryData, ZpData>;
template class DPFORAM<BinaryData, ZpDebugData>;

template class DPFORAM<ZpData, BinaryData>;
template class DPFORAM<ZpData, ECData>;
template class DPFORAM<ZpData, ZpData>;
template class DPFORAM<ZpData, ZpDebugData>;

template class DPFORAM<ECData, BinaryData>;
template class DPFORAM<ECData, ECData>;
template class DPFORAM<ECData, ZpData>;
template class DPFORAM<ECData, ZpDebugData>;

template class DPFORAM<ZpDebugData, BinaryData>;
template class DPFORAM<ZpDebugData, ECData>;
template class DPFORAM<ZpDebugData, ZpData>;
template class DPFORAM<ZpDebugData, ZpDebugData>;