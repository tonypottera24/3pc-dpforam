#include "oram.h"

template <typename K, typename D>
ORAM<K, D>::ORAM(const uint party, Peer peer[2], uint n, uint data_size) : party_(party), peer_(peer), n_(n) {
    debug_print("ORAM n = %u, data_size = %u\n", n, data_size);
    this->last_read_block_13_.Resize(data_size * DATA_PER_BLOCK);
    this->last_read_block_13_.Reset();
    this->last_read_data_13_.Resize(data_size);
    this->last_read_data_13_.Reset();
    this->InitArray(this->write_array_13_, n, data_size);

    // fprintf(stderr, "ORAM, n = %u, data_size = %u\n", n, data_size);
    // fprintf(stderr, "ORAM, array.size = %lu, array[0].Size = %u\n", this->write_array_13_.size(), this->write_array_13_[0].Size());
    // fprintf(stderr, "ORAM, total size = %lu\n", this->write_array_13_.size() * this->write_array_13_[0].Size() * 4);
    // fprintf(stderr, "\n");

    if (n > DATA_PER_BLOCK) {
        for (uint b = 0; b < 2; b++) {
            this->InitArray(this->read_array_23_[b], n, data_size);
        }

        uint pos_n = divide_ceil(n, DATA_PER_BLOCK);
        uint pos_data_size = byte_length(pos_n << 1);
        this->position_map_ = new ORAM<BinaryData, BinaryData>(party, peer, pos_n, pos_data_size);
    }
}

template <typename K, typename D>
ORAM<K, D>::~ORAM() {
    if (this->position_map_ != NULL) {
        delete this->position_map_;
    }
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b].clear();
        this->cache_array_23_[b].clear();
    }
    this->write_array_13_.clear();
}

template <typename K, typename D>
void ORAM<K, D>::Reset() {
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
void ORAM<K, D>::InitArray(std::vector<BulkData<D>> &array, const uint n, const uint data_size) {
    array.resize(divide_ceil(n, DATA_PER_BLOCK), BulkData<D>(data_size * DATA_PER_BLOCK));
}

template <typename K, typename D>
void ORAM<K, D>::ResetArray(std::vector<BulkData<D>> &array) {
    for (uint i = 0; i < array.size(); i++) {
        array[i].Reset();
    }
}

template <typename K, typename D>
void ORAM<K, D>::KeyToIndex(K key_23[2], uint index_23[2], Benchmark::Record *benchmark) {
    debug_print("[%u]KeyToIndex\n", this->Size());
    uint index_13 = PIR::DPF_KEY_PIR<K>(this->party_, this->peer_, this->fss_, this->key_array_13_, key_23, benchmark);
    ShareIndexTwoThird<K>(this->peer_, index_13, this->key_array_13_.size(), index_23, benchmark);
    // debug_print("[%u]KeyToIndex index_13 = %u, index_23 = (%u, %u)\n", this->Size(), index_13, index_23[0], index_23[1]);
}

template <typename K, typename D>
BulkData<D> ORAM<K, D>::GetLatestData(BulkData<D> &read_block_13, BulkData<D> &cache_block_13, const bool is_cached_23[2], Benchmark::Record *benchmark) {
    debug_print("[%u]GetLatestData, is_cached_23 = (%u, %u)\n", this->Size(), is_cached_23[0], is_cached_23[1]);

    uint data_size = read_block_13.Size();
    debug_print("data_size = %u\n", data_size);
    BulkData<D> out_block_13(data_size);
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;

        this->peer_[P0].WriteData(read_block_13, benchmark);
        this->peer_[P0].WriteData(cache_block_13, benchmark);

        SSOT::P2<BulkData<D>>(this->peer_, 2, data_size, benchmark);

        out_block_13.Random(this->peer_[P1].PRG());
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;
        BulkData<D> read_block_12(data_size);
        this->peer_[P2].ReadData(read_block_12);
        read_block_12 += read_block_13;

        BulkData<D> cache_block_12(data_size);
        this->peer_[P2].ReadData(cache_block_12);
        cache_block_12 += cache_block_13;

        std::vector<BulkData<D>> u = {read_block_12, cache_block_12};
        const uint b0 = is_cached_23[P1] ^ is_cached_23[P2];
        out_block_13 = SSOT::P0(this->peer_, b0, u, benchmark);
    } else if (this->party_ == 1) {
        const uint P2 = 1;
        std::vector<BulkData<D>> v = {read_block_13, cache_block_13};
        const uint b1 = is_cached_23[P2];
        out_block_13 = SSOT::P1(this->peer_, b1, v, benchmark);
        BulkData<D> tmp(data_size);
        tmp.Random(this->peer_[P2].PRG());
        out_block_13 -= tmp;
    }
    return out_block_13;
}

template <typename K, typename D>
void ORAM<K, D>::ReadPositionMap(const uint index_23[2], uint cache_index_23[2], bool is_cached_23[2], bool read_only, Benchmark::Record *benchmark) {
    debug_print("[%u]ReadPositionMap, index_23 = (%u, %u), read_only = %d\n", this->Size(), index_23[0], index_23[1], read_only);
    uint n = this->write_array_13_.size();
    // this->position_map_->PrintMetadata();

    // read block from array
    uint pos_data_size = this->position_map_->DataSize();
    BinaryData old_cache_index_13 = this->position_map_->Read(index_23, read_only, benchmark);
    BinaryData old_cache_index_23[2] = {BinaryData(pos_data_size), BinaryData(pos_data_size)};
    ShareTwoThird(this->peer_, old_cache_index_13, old_cache_index_23, benchmark);
    old_cache_index_13.Print("old_index_13");

    std::vector<uchar> dump;
    for (uint b = 0; b < 2; b++) {
        dump = old_cache_index_23[b].Dump();
        cache_index_23[b] = bytes_to_uint(dump.data(), dump.size());
        is_cached_23[b] = cache_index_23[b] & 1;
        cache_index_23[b] = (cache_index_23[b] >> 1) % n;
    }

    if (!read_only) {
        // TODO only 1 party need to write
        uint new_cache_index_uint = (this->cache_array_23_[0].size() << 1) + 1;
        debug_print("new_cache_index_uint = %u\n", new_cache_index_uint);

        uint data_size = byte_length(n << 1);
        std::vector<uchar> new_cache_index_bytes(data_size);
        uint_to_bytes(new_cache_index_uint, new_cache_index_bytes.data(), data_size);
        BinaryData new_cache_index_13;
        new_cache_index_13.Load(new_cache_index_bytes);
        new_cache_index_13.Print("new_cache_index_13");

        this->position_map_->Write(index_23, new_cache_index_13, benchmark);
        // this->position_map_->PrintMetadata();
    }
    // this->position_map_->PrintMetadata();
}

template <typename K, typename D>
D ORAM<K, D>::Read(const uint index_23[2], bool read_only, Benchmark::Record *benchmark) {
    debug_print("[%u]Read, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);

    uint block_index_23[2];
    uint data_index_23[2];
    for (uint b = 0; b < 2; b++) {
        block_index_23[b] = index_23[b] / DATA_PER_BLOCK;
        data_index_23[b] = index_23[b] % DATA_PER_BLOCK;
    }

    if (this->write_array_13_.size() == 1) {
        this->last_read_block_13_ = this->write_array_13_[0];
        // } else if (n <= SSOT_THRESHOLD) {
        //     return PIR::SSOT_PIR(this->party_, this->peer_, this->write_array_13_, index_23, !read_only);
    } else {
        this->last_read_block_13_ = DPFRead(block_index_23, read_only, benchmark);
    }
    this->last_read_block_13_.Print("this->last_read_block_13_");

    // std::vector<D> last_read_block_23[2];
    // ShareTwoThird(this->peer_, this->last_read_block_13_.data_, last_read_block_23, benchmark);
    // PIR::PIR(this->peer_, this->fss_, last_read_block_23, data_index_23, this->last_read_data_13_, benchmark);

    std::vector<D> last_read_block_13_data = this->last_read_block_13_.GetData();
    this->last_read_data_13_ = PIR::SSOT_PIR(this->party_, this->peer_, last_read_block_13_data, data_index_23, benchmark);

    this->last_read_data_13_.Print("this->last_read_data_13_");

    return this->last_read_data_13_;
}

template <typename K, typename D>
BulkData<D> ORAM<K, D>::DPFRead(const uint index_23[2], bool read_only, Benchmark::Record *benchmark) {
    debug_print("[%u]DPFRead, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);

    BulkData<D> v_read_13 = PIR::PIR(this->peer_, this->fss_, this->read_array_23_, index_23, benchmark);

    uint cache_index_23[2];
    bool is_cached_23[2];
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, is_cached_23, read_only, benchmark);
    }
    // debug_print("cache_index_23 = (%u, %u), is_cached_23 = (%u, %u)\n", cache_index_23[0], cache_index_23[1], is_cached_23[0], is_cached_23[1]);

    if (this->cache_array_23_->size() == 0) {
        return v_read_13;
    } else {
        BulkData<D> v_cache_13 = PIR::PIR(this->peer_, this->fss_, this->cache_array_23_, cache_index_23, benchmark);
        v_cache_13.Print("v_cache_13");

        return GetLatestData(v_read_13, v_cache_13, is_cached_23, benchmark);
    }
}

template <typename K, typename D>
void ORAM<K, D>::Write(const uint index_23[2], D &v_new_13, Benchmark::Record *benchmark) {
    debug_print("[%u]Write, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);

    uint block_index_23[2];
    uint data_index_23[2];
    for (uint b = 0; b < 2; b++) {
        block_index_23[b] = index_23[b] / DATA_PER_BLOCK;
        data_index_23[b] = index_23[b] % DATA_PER_BLOCK;
    }

    v_new_13.Print("v_new_13");
    this->last_read_data_13_.Print("this->last_read_data_13_");

    D v_delta_13 = v_new_13 - this->last_read_data_13_;

    BulkData<D> new_block_13 = this->last_read_block_13_;
    std::vector<D> new_block_13_data = new_block_13.GetData();

    PIW::PIW(this->party_, this->peer_, this->fss_, new_block_13_data, data_index_23, v_delta_13, benchmark);
    new_block_13.SetData(new_block_13_data);

    if (this->write_array_13_.size() == 1) {
        this->write_array_13_[0] = new_block_13;
    } else {
        DPFWrite(block_index_23, this->last_read_block_13_, new_block_13, benchmark);
    }
}

template <typename K, typename D>
void ORAM<K, D>::DPFWrite(const uint index_23[2], BulkData<D> &old_block_13, BulkData<D> &new_block_13, Benchmark::Record *benchmark) {
    debug_print("[%u]DPFWrite, index_23 = (%u, %u)\n", this->Size(), index_23[0], index_23[1]);

    BulkData<D> delta_block_13 = new_block_13 - old_block_13;

    PIW::PIW(this->party_, this->peer_, this->fss_, this->write_array_13_, index_23, delta_block_13, benchmark);
    this->AppendCache(new_block_13, benchmark);
}

template <typename K, typename D>
void ORAM<K, D>::AppendCache(BulkData<D> &new_block_13, Benchmark::Record *benchmark) {
    debug_print("[%u]AppendCache\n", this->Size());
    uint data_size = new_block_13.Size();
    BulkData<D> new_block_23[2] = {BulkData<D>(data_size), BulkData<D>(data_size)};
    ShareTwoThird(this->peer_, new_block_13, new_block_23, benchmark);
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].push_back(new_block_23[b]);
    }
    if (this->cache_array_23_[0].size() == this->read_array_23_[0].size()) {
        this->Flush(benchmark);
        if (this->position_map_ != NULL) {
            this->position_map_->Reset();
        }
    }
}

template <typename K, typename D>
void ORAM<K, D>::Flush(Benchmark::Record *benchmark) {
    debug_print("[%u]Flush\n", this->Size());
    uint size = this->write_array_13_.size();
    std::vector<BulkData<D>> array_23[2];
    array_23[0].resize(size);
    array_23[1].resize(size);
    ShareTwoThird(this->peer_, this->write_array_13_, array_23, benchmark);
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b] = array_23[b];
    }
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].clear();
    }
}

template <typename K, typename D>
void ORAM<K, D>::PrintMetadata() {
#ifdef DEBUG
    debug_print("========== PrintMetadata ==========\n");
    debug_print("party_: %u\n", this->party_);
    debug_print("DATA_PER_BLOCK: %u\n", DATA_PER_BLOCK);
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
void ORAM<K, D>::Test(uint iterations) {
    // TODO remember to free memory
    fprintf(stderr, "Test, iterations = %u \n", iterations);

    bool key_value = !std::is_same<K, BinaryData>::value;

    uint n = this->Size();

    // socket seems need extra setup at first read/write
    this->peer_[0].WriteUInt(0, NULL);
    this->peer_[1].WriteUInt(0, NULL);
    this->peer_[0].ReadUInt();
    this->peer_[1].ReadUInt();

    if (key_value) {
        uint key_size = K().Size();
        std::vector<K> key_array_33[3];
        for (uint b = 0; b < 3; b++) {
            key_array_33[b].resize(n, K(key_size));
        }
        for (uint i = 0; i < n; i++) {
            key_array_33[2][i].Random();
        }

        write_read_data(this->peer_[0], key_array_33[2], this->peer_[1], key_array_33[0], NULL);
        write_read_data(this->peer_[1], key_array_33[2], this->peer_[0], key_array_33[1], NULL);

        this->key_array_13_.resize(n);
        for (uint i = 0; i < n; i++) {
            this->key_array_13_[i] = key_array_33[0][i] + key_array_33[1][i] + key_array_33[2][i];
        }
    }

    for (uint iteration = 0; iteration < iterations; iteration++) {
        debug_print("\nTest, iteration = %u\n", iteration);

        uint index_23[2];
        uint key_size = K().Size();
        K key_23[2] = {K(key_size), K(key_size)};

        if (key_value) {
            // key_23[0].Random(this->peer_[0].PRG());
            // key_23[1].Random(this->peer_[1].PRG());
            uint index_33[3];
            index_33[2] = rand_uint() % n;
            this->peer_[0].WriteUInt(index_33[2], NULL);
            this->peer_[1].WriteUInt(index_33[2], NULL);
            index_33[0] = this->peer_[0].ReadUInt();
            index_33[1] = this->peer_[1].ReadUInt();
            uint index = (index_33[0] ^ index_33[1] ^ index_33[2]) % n;
            if (this->party_ == 2) {
                key_23[0].Random(this->peer_[0].PRG());
                key_23[1].Random(this->peer_[1].PRG());
            } else {
                key_23[this->party_].Random(this->peer_[this->party_].PRG());
                this->peer_[1 - this->party_].WriteData(key_23[this->party_], NULL);
                K k(key_size);
                this->peer_[1 - this->party_].ReadData(k);
                key_23[1 - this->party_] = this->key_array_13_[index] - key_23[this->party_] - k;
            }
            key_23[0].Print("key_23[0]");
            key_23[1].Print("key_23[1]");

            Benchmark::KEY_TO_INDEX.Start();
            KeyToIndex(key_23, index_23, &Benchmark::KEY_TO_INDEX);
            Benchmark::KEY_TO_INDEX.End();
            debug_print("index = %u, index_23 = (%u, %u)\n", index, index_23[0], index_23[1]);
        } else {
            index_23[0] = rand_uint(this->peer_[0].PRG()) % n;
            index_23[1] = rand_uint(this->peer_[1].PRG()) % n;
            // debug_print( "Test, rand_range = %llu, index_13 = %llu, index_23 = (%llu, %llu)\n", rand_range, index_13, index_23[0], index_23[1]);
        }

        // this->PrintMetadata();

        this->peer_[0].WriteUInt(0, NULL);
        this->peer_[1].WriteUInt(0, NULL);
        this->peer_[0].ReadUInt();
        this->peer_[1].ReadUInt();

        uint data_size = this->DataSize();

        // fprintf(stderr, "\nTest, ========== Read old data  ==========\n");
        debug_print("\nTest, ========== Read old data ==========\n");
        Benchmark::ORAM_READ.Start();
        D old_data_13 = this->Read(index_23, false, &Benchmark::ORAM_READ);
        Benchmark::ORAM_READ.End();

        old_data_13.Print("old_data_13");

        this->PrintMetadata();

        this->peer_[0].WriteUInt(0, NULL);
        this->peer_[1].WriteUInt(0, NULL);
        this->peer_[0].ReadUInt();
        this->peer_[1].ReadUInt();

        // fprintf(stderr, "\nTest, ========== Write random data ==========\n");
        debug_print("\nTest, ========== Write random data ==========\n");
        std::vector<D> new_data_13(3, D(data_size));
        new_data_13[2].Random();
        new_data_13[2].Print("new_data_13[2]");

        Benchmark::ORAM_WRITE.Start();
        this->Write(index_23, new_data_13[2], &Benchmark::ORAM_WRITE);
        Benchmark::ORAM_WRITE.End();
        // printf("Write random data party_time = %llu\n", party_time);

        this->PrintMetadata();

        // fprintf(stderr, "\nTest, ========== Read validation data  ==========\n");
        debug_print("\nTest, ========== Read validation data ==========\n");
        if (key_value) {
            KeyToIndex(key_23, index_23, NULL);
        }

        std::vector<D> verify_data_13(3, D(data_size));
        verify_data_13[2] = this->Read(index_23, true, NULL);

        verify_data_13[2].Print("verify_data_13[2]");

        this->PrintMetadata();

        this->peer_[0].WriteData(verify_data_13[2], NULL);
        this->peer_[1].WriteData(verify_data_13[2], NULL);
        this->peer_[0].ReadData(verify_data_13[0]);
        this->peer_[1].ReadData(verify_data_13[1]);
        D verify_data = verify_data_13[0] + verify_data_13[1] + verify_data_13[2];
        verify_data.Print("verify_data");

        this->peer_[0].WriteData(new_data_13[2], NULL);
        this->peer_[1].WriteData(new_data_13[2], NULL);
        this->peer_[0].ReadData(new_data_13[0]);
        this->peer_[1].ReadData(new_data_13[1]);
        D new_data = new_data_13[0] + new_data_13[1] + new_data_13[2];
        new_data.Print("new_data");

        if (verify_data == new_data) {
            debug_print("%u, Pass\n", iteration);
        } else {
            fprintf(stderr, "%u, Fail !!!\n", iteration);
            exit(1);
        }
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "n = %u\n", this->Size());
    fprintf(stderr, "\n");

#ifdef BENCHMARK_DPF
    Benchmark::DPF_GEN.PrintTotal(this->peer_, "DPF_GEN", iterations);
    Benchmark::DPF_EVAL.PrintTotal(this->peer_, "DPF_EVAL", iterations);
    Benchmark::DPF_EVAL_ALL.PrintTotal(this->peer_, "DPF_EVAL_ALL", iterations);
    fprintf(stderr, "\n");

    Benchmark::PSEUDO_DPF_GEN.PrintTotal(this->peer_, "PSEUDO_DPF_GEN", iterations);
    Benchmark::PSEUDO_DPF_EVAL.PrintTotal(this->peer_, "PSEUDO_DPF_EVAL", iterations);
    Benchmark::PSEUDO_DPF_EVAL_ALL.PrintTotal(this->peer_, "PSEUDO_DPF_EVAL_ALL", iterations);
    fprintf(stderr, "\n");
#endif

    Benchmark::KEY_TO_INDEX.PrintTotal(this->peer_, "KEY_TO_INDEX", iterations);
    Benchmark::ORAM_READ.PrintTotal(this->peer_, "ORAM_READ", iterations);
    Benchmark::ORAM_WRITE.PrintTotal(this->peer_, "ORAM_WRITE", iterations);
    fprintf(stderr, "\n");

    Benchmark::Record total = Benchmark::ORAM_READ + Benchmark::ORAM_WRITE + Benchmark::KEY_TO_INDEX;
    total.PrintTotal(this->peer_, "total", iterations);

    fprintf(stderr, "\n");
}

template class ORAM<BinaryData, BinaryData>;
template class ORAM<BinaryData, ECData>;
template class ORAM<BinaryData, ZpData>;
template class ORAM<BinaryData, ZpDebugData>;

template class ORAM<ZpData, BinaryData>;
template class ORAM<ZpData, ECData>;
template class ORAM<ZpData, ZpData>;
template class ORAM<ZpData, ZpDebugData>;

template class ORAM<ECData, BinaryData>;
template class ORAM<ECData, ECData>;
template class ORAM<ECData, ZpData>;
template class ORAM<ECData, ZpDebugData>;

template class ORAM<ZpDebugData, BinaryData>;
template class ORAM<ZpDebugData, ECData>;
template class ORAM<ZpDebugData, ZpData>;
template class ORAM<ZpDebugData, ZpDebugData>;