#include "dpf_oram.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const uint party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
                 uint64_t n, uint64_t data_size, uint64_t tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold) : Protocol(party, connections, rnd, prgs) {
    // fprintf(stderr, "DPFORAM n = %llu, data_size = %u, tau = %u\n", n, data_size, tau);
    this->tau_ = tau;
    this->ssot_threshold_ = ssot_threshold;
    this->pseudo_dpf_threshold_ = pseudo_dpf_threshold;

    this->InitArray(this->write_array_13_, n, data_size, true);

    if (n > 1ULL) {
        for (uint b = 0; b < 2; b++) {
            this->InitArray(this->read_array_23_[b], n, data_size);
        }

        uint64_t pos_data_per_block = 1 << this->tau_;
        uint64_t pos_n = uint64_ceil_divide(n, pos_data_per_block);
        uint64_t pos_data_size = byte_length(n << 1) * pos_data_per_block;
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, pos_n, pos_data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    }
}

DPFORAM::~DPFORAM() {
    if (this->position_map_ != NULL) {
        delete this->position_map_;
    }
    for (uint b = 0; b < 2; b++) {
        this->read_array_23_[b].clear();
        this->cache_array_23_[b].clear();
    }
    this->write_array_13_.clear();
}

void DPFORAM::Reset() {
    for (uint b = 0; b < 2; b++) {
        ResetArray(read_array_23_[b]);
        cache_array_23_[b].clear();
    }
    ResetArray(write_array_13_);
    if (this->position_map_ != NULL) {
        this->position_map_->Reset();
    }
}

void DPFORAM::InitArray(std::vector<BinaryData> &array, const uint64_t n, const uint64_t data_size, bool set_zero) {
    for (uint64_t i = 0; i < n; i++) {
        array.emplace_back(data_size, set_zero);
    }
}

void DPFORAM::ResetArray(std::vector<BinaryData> &array) {
    for (uint64_t i = 0; i < array.size(); i++) {
        array[i].Reset();
    }
}

void DPFORAM::PrintArray(std::vector<BinaryData> &array, const char *array_name, const int64_t array_index) {
    if (array_index == -1) {
        printf("%s: ", array_name);
    } else {
        printf("%s[%llu]: ", array_name, array_index);
    }
    for (uint64_t i = 0; i < array.size(); i++) {
        array[i].Print();
    }
}

uint64_t DPFORAM::Size() {
    return this->write_array_13_.size();
}

uint DPFORAM::DataSize() {
    return this->write_array_13_[0].Size();
}

BinaryData &DPFORAM::PIR(std::vector<BinaryData> array_23[2], const uint64_t index_23[2], bool count_band) {
    uint64_t n = array_23[0].size();
    fprintf(stderr, "[%llu]PIR, n = %llu\n", this->Size(), n);
    if (n == 1) {
        BinaryData *v_out_13 = new BinaryData();
        *v_out_13 = array_23[0][0] - array_23[1][0];
        return *v_out_13;
    } else if (n <= this->pseudo_dpf_threshold_) {
        return PSEUDO_DPF_PIR(array_23, index_23, count_band);
    } else {
        return DPF_PIR(array_23, index_23, count_band);
    }
}

BinaryData &DPFORAM::DPF_PIR(std::vector<BinaryData> array_23[2], const uint64_t index_23[2], bool count_band) {
    uint64_t n = array_23[0].size();
    fprintf(stderr, "[%llu]DPF_PIR, n = %llu\n", this->Size(), n);
    uint64_t log_n = uint64_log2(n);
    n = 1ULL << (log_n + 1ULL);

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);
    // TODO check if 0 is always has more 1

    this->conn_[0]->Write(query_23[0], query_size, count_band);
    this->conn_[1]->Write(query_23[1], query_size, count_band);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    uint data_size = array_23[0][0].Size();
    BinaryData *v_out_13 = new BinaryData(data_size, true);
    for (uint b = 0; b < 2; b++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[b], log_n, dpf_out);
        for (uint64_t i = 0; i < array_23[b].size(); i++) {
            if (dpf_out[i ^ index_23[b]]) {
                *v_out_13 += array_23[b][i];
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] query_23[b];
    }
    return *v_out_13;
}

BinaryData &DPFORAM::PSEUDO_DPF_PIR(std::vector<BinaryData> array_23[2], const uint64_t index_23[2], bool count_band) {
    uint64_t n = array_23[0].size();
    fprintf(stderr, "[%llu]PSEUDO_DPF_PIR, n = %llu, index_23 = (%llu, %llu)\n", this->Size(), n, index_23[0], index_23[1]);
    uint64_t byte_length = uint64_ceil_divide(n, 8ULL);
    uchar *dpf_out[2];
    for (uint b = 0; b < 2; b++) {
        dpf_out[b] = new uchar[byte_length];
    }
    this->fss_.PseudoGen(&this->prgs_[0], index_23[0] ^ index_23[1], byte_length, dpf_out[1]);
    this->conn_[1]->Write(dpf_out[1], byte_length, count_band);
    this->conn_[0]->Read(dpf_out[1], byte_length);
    this->prgs_[1].GenerateBlock(dpf_out[0], byte_length);

    uint data_size = array_23[0][0].Size();
    BinaryData *v_out_13 = new BinaryData(data_size, true);
    for (uint b = 0; b < 2; b++) {
        for (uint64_t i = 0; i < array_23[b].size(); i++) {
            if (this->fss_.PseudoEval(i ^ index_23[b], dpf_out[b])) {
                *v_out_13 += array_23[b][i];
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] dpf_out[b];
    }
    return *v_out_13;
}

BinaryData &DPFORAM::SSOT_PIR(std::vector<BinaryData> &array_13, const uint64_t index_23[2], bool count_band) {
    // TODO n may not be power of 2
    uint64_t n = array_13.size();
    fprintf(stderr, "[%llu]SSOT_PIR, n = %llu, index_23 = (%llu, %llu)\n", this->Size(), n, index_23[0], index_23[1]);
    uint data_size = array_13[0].Size();
    BinaryData *v_out_13;
    SSOT *ssot = new SSOT(this->party_, this->conn_, this->rnd_, this->prgs_);
    if (this->party_ == 2) {
        // const uint P1 = 0, P0 = 1;
        const uint P0 = 1;

        this->conn_[P0]->WriteData(array_13, count_band);

        ssot->P2(n, data_size, count_band);

        v_out_13 = new BinaryData(data_size, true);
        v_out_13->Random(this->prgs_[P0]);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;

        std::vector<BinaryData> u = this->conn_[P2]->ReadData(n, data_size);
        for (uint64_t i = 0; i < n; i++) {
            u[i] += array_13[i];
        }
        v_out_13 = ssot->P0(index_23[P1] ^ index_23[P2], u, count_band);

        BinaryData tmp = BinaryData(data_size);
        tmp.Random(this->prgs_[P2]);
        *v_out_13 += tmp;
    } else {  // this->party_ == 1
        // const uint P0 = 0, P2 = 1;
        const uint P2 = 1;
        v_out_13 = ssot->P1(index_23[P2], array_13, count_band);
    }
    delete ssot;
    return *v_out_13;
}

void DPFORAM::PIW(std::vector<BinaryData> &array_13, const uint64_t index_23[2], BinaryData v_delta_23[2], bool count_band) {
    uint64_t n = array_13.size();
    fprintf(stderr, "[%llu]PIW, index_23 = (%llu, %llu), n = %llu\n", this->Size(), index_23[0], index_23[1], n);
    if (n == 1) {
        array_13[0] += v_delta_23[0];
    } else if (n <= this->pseudo_dpf_threshold_) {
        PSEUDO_DPF_PIW(array_13, index_23, v_delta_23, count_band);
    } else {
        DPF_PIW(array_13, index_23, v_delta_23, count_band);
    }
}

void DPFORAM::DPF_PIW(std::vector<BinaryData> &array_13, const uint64_t index_23[2], BinaryData v_delta_23[2], bool count_band) {
    uint64_t n = array_13.size();
    fprintf(stderr, "[%llu]DPF_PIW, index_23 = (%llu, %llu), n = %llu\n", this->Size(), index_23[0], index_23[1], n);
    uint64_t log_n = uint64_log2(n);
    n = 1ULL << (log_n + 1ULL);
    // fprintf(stderr, "[%llu]PIW, log_n = %llu\n", this->n_, log_n);

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    this->conn_[0]->Write(query_23[0], query_size, count_band);
    this->conn_[1]->Write(query_23[1], query_size, count_band);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    for (uint b = 0; b < 2; b++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[b], log_n, dpf_out);
        for (uint64_t i = 0; i < array_13.size(); i++) {
            // fprintf(stderr, "PIW i = %u, j = %llu, jj = %llu, dpf = %u\n", i, j, j ^ index_23[i], dpf_out[j ^ index_23[i]]);
            if (dpf_out[i ^ index_23[b]]) {
                array_13[i] += v_delta_23[b];
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] query_23[b];
    }
}

void DPFORAM::PSEUDO_DPF_PIW(std::vector<BinaryData> &array_13, const uint64_t index_23[2], BinaryData v_delta_23[2], bool count_band) {
    uint64_t n = array_13.size();
    fprintf(stderr, "[%llu]PSEUDO_DPF_PIW, index_23 = (%llu, %llu), n = %llu\n", this->Size(), index_23[0], index_23[1], n);
    uint64_t byte_length = uint64_ceil_divide(n, 8ULL);
    uchar *dpf_out[2];
    for (uint b = 0; b < 2; b++) {
        dpf_out[b] = new uchar[byte_length];
    }

    this->fss_.PseudoGen(&this->prgs_[0], index_23[0] ^ index_23[1], byte_length, dpf_out[1]);
    this->conn_[1]->Write(dpf_out[1], byte_length, count_band);
    this->conn_[0]->Read(dpf_out[1], byte_length);
    this->prgs_[1].GenerateBlock(dpf_out[0], byte_length);

    for (uint b = 0; b < 2; b++) {
        for (uint64_t i = 0; i < array_13.size(); i++) {
            // fprintf(stderr, "PIW i = %u, j = %llu, jj = %llu, dpf = %u\n", i, j, j ^ index_23[i], dpf_out[j ^ index_23[i]]);
            if (this->fss_.PseudoEval(i ^ index_23[b], dpf_out[b])) {
                array_13[i] += v_delta_23[b];
            }
        }
    }

    for (uint b = 0; b < 2; b++) {
        delete[] dpf_out[b];
    }
}

BinaryData *DPFORAM::GetLatestData(BinaryData &v_read_13,
                                   BinaryData &v_cache_13, const bool is_cached_23[2], bool count_band) {
    fprintf(stderr, "[%llu]GetLatestData, is_cached_23 = (%u, %u)\n", this->Size(), is_cached_23[0], is_cached_23[1]);

    uint data_size = v_read_13.Size();
    BinaryData *v_out_23 = new BinaryData[2]{BinaryData(data_size), BinaryData(data_size)};
    SSOT *ssot = new SSOT(this->party_, this->conn_, this->rnd_, this->prgs_);
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;

        this->conn_[P0]->WriteData(v_read_13, count_band);
        this->conn_[P0]->WriteData(v_cache_13, count_band);

        ssot->P2(2, data_size, count_band);

        v_out_23[P0] = this->conn_[P0]->ReadData(data_size);
        v_out_23[P1] = this->conn_[P1]->ReadData(data_size);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;
        BinaryData v_read_12 = this->conn_[P2]->ReadData(data_size) + v_read_13;
        BinaryData v_cache_12 = this->conn_[P2]->ReadData(data_size) + v_cache_13;

        std::vector<BinaryData> u = {v_read_12, v_cache_12};
        const uint64_t b0 = is_cached_23[P1] ^ is_cached_23[P2];
        v_out_23[P2] = *ssot->P0(b0, u, count_band);

        v_out_23[P1].Random(this->prgs_[P1]);

        v_out_23[P2] += v_out_23[P1];
        this->conn_[P2]->WriteData(v_out_23[P2], count_band);
    } else if (this->party_ == 1) {
        const uint P0 = 0, P2 = 1;
        std::vector<BinaryData> v = {v_read_13, v_cache_13};
        const uint64_t b1 = is_cached_23[P2];
        v_out_23[P2] = *ssot->P1(b1, v, count_band);

        v_out_23[P0].Random(this->prgs_[P0]);
        this->conn_[P2]->WriteData(v_out_23[P2], count_band);
    }
    delete ssot;
    return v_out_23;
}

BinaryData *DPFORAM::ShareTwoThird(BinaryData &v_in_13, bool count_band) {
    fprintf(stderr, "[%llu]ShareTwoThird\n", this->Size());
    this->conn_[1]->WriteData(v_in_13, count_band);
    BinaryData v_out = this->conn_[0]->ReadData(v_in_13.Size());
    return new BinaryData[2]{v_out, v_in_13};
}

void DPFORAM::ShareIndexTwoThird(const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band) {
    fprintf(stderr, "[%llu]ShareIndexTwoThird, n = %llu, index_13 = %llu\n", this->Size(), n, index_13);
    uint64_t rand_range = n;
    this->conn_[1]->WriteLong(index_13, count_band);
    index_23[0] = this->conn_[0]->ReadLong() % rand_range;
    index_23[1] = index_13 % rand_range;
}

void DPFORAM::ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached_23[2], bool read_only) {
    fprintf(stderr, "[%llu]ReadPositionMap, index_23 = (%llu, %llu), read_only = %d\n", this->Size(), index_23[0], index_23[1], read_only);

    uint64_t n = this->Size();
    uint64_t data_size = byte_length((n << 1ULL) + 1ULL);
    uint64_t data_per_block = 1ULL << this->tau_;
    // fprintf(stderr, "[%llu]ReadPositionMap, data_size = %llu, data_per_block = %llu\n", this->n_, data_size, data_per_block);

    uint64_t block_index_23[2];
    uint64_t data_index_23[2];
    for (uint b = 0; b < 2; b++) {
        block_index_23[b] = index_23[b] / data_per_block;
        data_index_23[b] = index_23[b] % data_per_block;
    }

    // read block from array
    BinaryData *old_block_23 = this->position_map_->Read(block_index_23, read_only);

    // read data from block
    std::vector<BinaryData> old_data_array_23[2];
    for (uint b = 0; b < 2; b++) {
        uchar *old_block_data = old_block_23[b].Dump();
        for (uint i = 0; i < data_per_block; i++) {
            old_data_array_23[b].emplace_back(&old_block_data[i * data_size], data_size);
        }
    }
    BinaryData old_data_13 = PIR(old_data_array_23, data_index_23, !read_only);
    BinaryData *old_data_23 = this->ShareTwoThird(old_data_13, !read_only);

    for (uint b = 0; b < 2; b++) {
        cache_index_23[b] = bytes_to_uint64(old_data_23[b].Dump(), old_data_23[b].Size());
        is_cached_23[b] = cache_index_23[b] & 1;
        cache_index_23[b] = (cache_index_23[b] >> 1) % n;
    }

    if (read_only == false) {
        // TODO only 1 party need to write
        BinaryData new_block_13 = old_block_23[0];

        std::vector<BinaryData> data_array_13;
        uchar *new_block_data = new_block_13.Dump();
        for (uint i = 0; i < data_per_block; i++) {
            data_array_13.emplace_back(&new_block_data[i * data_size], data_size);
        }
        uint64_t new_cache_index = (this->cache_array_23_[0].size() << 1) + 1ULL;
        BinaryData new_data_13 = BinaryData((uchar *)&new_cache_index, data_size);

        for (uint b = 0; b < 2; b++) {
            old_data_23[b] += new_data_13;
        }
        PIW(data_array_13, data_index_23, old_data_23, !read_only);

        BinaryData *new_block_23 = this->ShareTwoThird(new_block_13, !read_only);
        this->position_map_->Write(block_index_23, old_block_23, new_block_23, !read_only);

        delete[] new_block_23;
    }
    delete[] old_data_23;
}

BinaryData *DPFORAM::Read(const uint64_t index_23[2], bool read_only) {
    fprintf(stderr, "[%llu]Read, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);
    uint64_t n = this->Size();
    if (n == 1) {
        return this->ShareTwoThird(this->write_array_13_[0], !read_only);
    } else if (n >= this->ssot_threshold_) {
        return SSOT_Read(index_23, read_only);
    } else {
        return DPF_Read(index_23, read_only);
    }
}

BinaryData *DPFORAM::DPF_Read(const uint64_t index_23[2], bool read_only) {
    fprintf(stderr, "[%llu]DPF_Read, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);

    BinaryData v_read_13 = PIR(this->read_array_23_, index_23, !read_only);

    uint64_t cache_index_23[2];
    bool is_cached_23[2];
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, is_cached_23, read_only);
    }

    if (this->cache_array_23_->size() == 0) {
        return this->ShareTwoThird(v_read_13, !read_only);
    }
    BinaryData v_cache_13 = PIR(this->cache_array_23_, cache_index_23, !read_only);
    return GetLatestData(v_read_13, v_cache_13, is_cached_23, !read_only);
}

BinaryData *DPFORAM::SSOT_Read(const uint64_t index_23[2], bool read_only) {
    fprintf(stderr, "[%llu]SSOT_Read, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);
    BinaryData v_read_13 = SSOT_PIR(this->write_array_13_, index_23, !read_only);
    return this->ShareTwoThird(v_read_13, !read_only);
}

void DPFORAM::Write(const uint64_t index_23[2], BinaryData old_data_23[2], BinaryData new_data_23[2], bool count_band) {
    fprintf(stderr, "[%llu]Write, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);
    uint64_t n = this->write_array_13_.size();
    if (n == 1) {
        this->write_array_13_[0] = new_data_23[0];
    } else {
        DPF_Write(index_23, old_data_23, new_data_23, count_band);
    }
}

void DPFORAM::DPF_Write(const uint64_t index_23[2], BinaryData old_data_23[2], BinaryData new_data_23[2], bool count_band) {
    fprintf(stderr, "[%llu]DPF_Write, index_23 = (%llu, %llu)\n", this->Size(), index_23[0], index_23[1]);

    BinaryData delta_data_23[2];
    for (uint b = 0; b < 2; b++) {
        delta_data_23[b] = old_data_23[b] + new_data_23[b];
    }

    PIW(this->write_array_13_, index_23, delta_data_23, count_band);
    this->AppendCache(new_data_23, count_band);
}

void DPFORAM::AppendCache(BinaryData v_new_23[2], bool count_band) {
    fprintf(stderr, "[%llu]AppendCache\n", this->Size());
    for (uint b = 0; b < 2; b++) {
        this->cache_array_23_[b].push_back(v_new_23[b]);
    }
    if (this->cache_array_23_[0].size() == this->read_array_23_[0].size()) {
        this->Flush();
        if (this->position_map_ != NULL) {
            this->position_map_->Reset();
        }
    }
}

void DPFORAM::Flush(bool count_band) {
    fprintf(stderr, "[%llu]Flush\n", this->Size());
    for (uint64_t i = 0; i < this->write_array_13_.size(); i++) {
        BinaryData *array_23 = this->ShareTwoThird(this->write_array_13_[i], count_band);
        for (uint b = 0; b < 2; b++) {
            this->read_array_23_[b][i] = array_23[b];
        }
    }
}

void DPFORAM::PrintMetadata() {
    fprintf(stderr, "========== PrintMetadata ==========\n");
    fprintf(stderr, "party_: %u\n", this->party_);
    fprintf(stderr, "tau_: %llu\n", this->tau_);
    fprintf(stderr, "n: %lu\n", this->write_array_13_.size());
    fprintf(stderr, "data_size: %u\n", this->DataSize());
    fprintf(stderr, "cache_size: %lu\n", this->cache_array_23_[0].size());
    fprintf(stderr, "position_map_: %d\n", (this->position_map_ != NULL));

    // this->PrintArray(this->read_array_23_[0], "read_array_23", 0);
    // this->PrintArray(this->read_array_23_[1], "read_array_23", 1);
    // this->PrintArray(this->cache_array_23_[0], "cache_23", 0);
    // this->PrintArray(this->cache_array_23_[1], "cache_23", 1);
    // this->PrintArray(this->write_array_13_, "write_array");

    fprintf(stderr, "========== PrintMetadata ==========\n");
}

void DPFORAM::Test(uint iterations) {
    // TODO remember to free memory
    fprintf(stderr, "Test, iterations = %u \n", iterations);
    uint64_t party_time = 0;
    uint64_t time;

    for (uint iteration = 0; iteration < iterations; iteration++) {
        fprintf(stderr, "Test, iteration = %u\n", iteration);
        uint64_t n = this->write_array_13_.size();
        uint64_t rand_range = n - 1;
        uint64_t index_13 = rand_uint64() % rand_range;
        uint64_t index_23[2];
        this->ShareIndexTwoThird(index_13, n, index_23, false);
        // fprintf(stderr, "Test, rand_range = %llu, index_13 = %llu, index_23 = (%llu, %llu)\n", rand_range, index_13, index_23[0], index_23[1]);

        fprintf(stderr, "\nTest, ========== Read old data ==========\n");
        time = timestamp();
        BinaryData *old_data_23 = this->Read(index_23);
        party_time += timestamp() - time;

        fprintf(stderr, "\nTest, ========== Write random data ==========\n");
        BinaryData new_data_23[2] = {BinaryData(this->DataSize()), BinaryData(this->DataSize())};
        for (uint b = 0; b < 2; b++) {
            new_data_23[b].Random(this->prgs_[b]);
        }
        time = timestamp();
        this->Write(index_23, old_data_23, new_data_23);
        party_time += timestamp() - time;

        fprintf(stderr, "\nTest, ========== Read validation data ==========\n");
        BinaryData *verify_data_23 = this->Read(index_23, true);

        this->conn_[1]->WriteData(verify_data_23[0], false);
        BinaryData verify_data = this->conn_[0]->ReadData(this->DataSize());
        verify_data += verify_data_23[0] + verify_data_23[1];

        this->conn_[1]->WriteData(new_data_23[0], false);
        BinaryData new_data = this->conn_[0]->ReadData(this->DataSize());
        new_data += new_data_23[0] + new_data_23[1];

        if (verify_data == new_data) {
            // fprintf(stderr, "%u, Pass\n", iteration);
        } else {
            fprintf(stderr, "%u, Fail !!!\n", iteration);
            exit(1);
        }
    }

    uint64_t party_bandwidth = Bandwidth();
    this->conn_[0]->WriteLong(party_bandwidth, false);
    this->conn_[1]->WriteLong(party_bandwidth, false);
    uint64_t total_bandwidth = party_bandwidth;
    total_bandwidth += this->conn_[0]->ReadLong();
    total_bandwidth += this->conn_[1]->ReadLong();

    conn_[0]->WriteLong(party_time, false);
    conn_[1]->WriteLong(party_time, false);
    uint64_t max_time = party_time;
    max_time = std::max(max_time, conn_[0]->ReadLong());
    max_time = std::max(max_time, conn_[1]->ReadLong());

    fprintf(stderr, "\n");
    fprintf(stderr, "n = %lu\n", this->write_array_13_.size());
    fprintf(stderr, "Party Bandwidth(byte): %llu\n", party_bandwidth / iterations);
    fprintf(stderr, "Party execution time(microsec): %llu\n", party_time / iterations);
    fprintf(stderr, "Total Bandwidth(byte): %llu\n", total_bandwidth / iterations);
    fprintf(stderr, "Max Execution time(microsec): %llu\n", max_time / iterations);
    fprintf(stderr, "\n");
}
