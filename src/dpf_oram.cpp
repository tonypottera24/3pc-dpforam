#include "dpf_oram.h"

FSS1Bit DPFORAM::fss_;

DPFORAM::DPFORAM(const uint party, Connection *connections[2],
                 CryptoPP::AutoSeededRandomPool *rnd,
                 CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption *prgs,
                 uint64_t n, uint64_t data_size, uint64_t tau, uint64_t ssot_threshold, uint64_t pseudo_dpf_threshold) : Protocol(party, connections, rnd, prgs) {
    // fprintf(stderr, "DPFORAM n = %llu, data_size = %u, tau = %u\n", n, data_size, tau);
    this->n_ = n;
    this->data_size_ = data_size;
    this->tau_ = tau;
    this->ssot_threshold_ = ssot_threshold;
    this->pseudo_dpf_threshold_ = pseudo_dpf_threshold;

    this->InitArray(this->write_array_13_, true);

    if (this->n_ > 1ULL) {
        for (uint i = 0; i < 2; i++) {
            this->InitArray(this->read_array_23_[i]);
            this->InitArray(this->cache_23_[i]);
        }
        this->cache_ctr_ = 0;

        uint64_t pos_data_per_block = 1 << this->tau_;
        uint64_t pos_n = uint64_ceil_divide(this->n_, pos_data_per_block);
        uint64_t pos_data_size = byte_length(this->n_ << 1) * pos_data_per_block;
        this->position_map_ = new DPFORAM(party, connections, rnd, prgs, pos_n, pos_data_size, tau, ssot_threshold, pseudo_dpf_threshold);
    }
}

DPFORAM::~DPFORAM() {
    if (this->position_map_ != NULL) {
        delete this->position_map_;
    }
    for (uint i = 0; i < 2; i++) {
        this->DeleteArray(this->read_array_23_[i]);
        this->DeleteArray(this->cache_23_[i]);
    }
    this->DeleteArray(this->write_array_13_);
}

void DPFORAM::Reset() {
    if (this->n_ > 1) {
        for (uint i = 0; i < 2; i++) {
            for (uint64_t j = 0; j < this->n_; j++) {
                memset(read_array_23_[i][j], 0, this->data_size_);
                memset(cache_23_[i][j], 0, this->data_size_);
            }
        }
    }
    this->cache_ctr_ = 0;
    if (this->write_array_13_ != NULL) {
        for (uint64_t i = 0; i < this->n_; i++) {
            memset(write_array_13_[i], 0, this->data_size_);
        }
    }
    if (this->position_map_ != NULL) {
        this->position_map_->Reset();
    }
}

void DPFORAM::InitArray(uchar **&array, bool set_zero) {
    array = new uchar *[this->n_];
    for (uint64_t i = 0; i < this->n_; i++) {
        array[i] = new uchar[this->data_size_];
        if (set_zero) {
            memset(array[i], 0, this->data_size_);
        }
    }
}

void DPFORAM::ResetArray(uchar **array) {
    if (array == NULL) return;
    for (uint64_t i = 0; i < this->n_; i++) {
        memset(array[i], 0, this->data_size_);
    }
}

void DPFORAM::DeleteArray(uchar **array) {
    if (array == NULL) return;
    for (uint64_t i = 0; i < this->n_; i++) {
        delete[] array[i];
    }
    delete[] array;
}

void DPFORAM::PIR(uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band) {
    assert((n > 0) && "PIR n == 0");
    if (n == 1) {
        xor_bytes(array_23[0][0], array_23[1][0], data_size, v_out_13);
    } else if (n <= this->pseudo_dpf_threshold_) {
        PSEUDO_DPF_PIR(array_23, n, data_size, index_23, v_out_13, count_band);
    } else {
        DPF_PIR(array_23, n, data_size, index_23, v_out_13, count_band);
    }
}

void DPFORAM::DPF_PIR(uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band) {
    // fprintf(stderr, "[%llu]PIR, n = %llu\n", this->n_, n);
    uint64_t log_n = uint64_log2(n);

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    this->conn_[0]->Write(query_23[0], query_size, count_band);
    this->conn_[1]->Write(query_23[1], query_size, count_band);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    memset(v_out_13, 0, data_size);
    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[i], log_n, dpf_out);
        for (uint64_t j = 0; j < n; j++) {
            // fprintf(stderr, "PIR i = %u, j = %llu, jj = %llu, dpf = %u\n", i, j, j ^ index_23[i], dpf_out[j ^ index_23[i]]);
            if (dpf_out[j ^ index_23[i]]) {
                xor_bytes(v_out_13, array_23[i][j], data_size, v_out_13);
            }
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] query_23[i];
    }
}

void DPFORAM::PSEUDO_DPF_PIR(uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band) {
    // fprintf(stderr, "[%llu]PIR, n = %llu, index_23 = (%llu, %llu)\n", this->n_, n, index_23[0], index_23[1]);

    uint64_t byte_length = uint64_ceil_divide(n, 8ULL);
    uchar *dpf_out[2];
    for (uint i = 0; i < 2; i++) {
        dpf_out[i] = new uchar[byte_length];
    }
    this->fss_.PseudoGen(&this->prgs_[0], index_23[0] ^ index_23[1], byte_length, dpf_out[1]);
    this->conn_[1]->Write(dpf_out[1], byte_length, count_band);
    this->conn_[0]->Read(dpf_out[1], byte_length);
    this->prgs_[1].GenerateBlock(dpf_out[0], byte_length);

    memset(v_out_13, 0, data_size);
    for (uint i = 0; i < 2; i++) {
        for (uint64_t j = 0; j < n; j++) {
            // fprintf(stderr, "PIR i = %u, j = %llu, jj = %llu, jj_byte = %llu, jj_bit = %llu, dpf = %u\n", i, j, j ^ index_23[i], jj_byte, jj_bit, dpf_out[i][jj_byte] ^ (1 << jj_bit));
            if (this->fss_.PseudoEval(j ^ index_23[i], dpf_out[i])) {
                xor_bytes(v_out_13, array_23[i][j], data_size, v_out_13);
            }
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] dpf_out[i];
    }
}

void DPFORAM::SSOT_PIR(uchar **array_13, uchar **array_23[2], const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_out_13, bool count_band) {
    // fprintf(stderr, "[%llu]SSOT_PIR, v_meta_13 = %d\n", this->n_, v_meta_13[0]);

    SSOT *ssot = new SSOT(this->party_, this->conn_, this->rnd_, this->prgs_);
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;

        if (array_13 != NULL) {
            uchar array[n * data_size];
            for (uint64_t i = 0; i < n; i++) {
                memcpy(&array[i * data_size], array_13[i], data_size);
            }
            this->conn_[P0]->Write(array, n * data_size);
        }

        ssot->P2(n, data_size, count_band);

        this->prgs_[P0].GenerateBlock(v_out_13, data_size);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;

        const uint64_t b0 = index_23[P1] ^ index_23[P2];

        uchar *u[n];
        if (array_13 != NULL) {
            uchar array[n * data_size];
            this->conn_[P2]->Read(array, n * data_size);
            for (uint64_t i = 0; i < n; i++) {
                u[i] = new uchar[data_size];
                xor_bytes(&array[i * data_size], array_13[i], data_size, u[i]);
            }
        } else {
            for (uint64_t i = 0; i < n; i++) {
                u[i] = new uchar[data_size];
                xor_bytes(array_23[0][i], array_23[1][i], data_size, u[i]);
            }
        }
        ssot->P0(b0, u, n, data_size, v_out_13, count_band);

        uchar tmp[data_size];
        this->prgs_[P2].GenerateBlock(tmp, data_size);
        xor_bytes(tmp, v_out_13, data_size, v_out_13);
    } else if (this->party_ == 1) {
        const uint P0 = 0, P2 = 1;
        const uint64_t b1 = index_23[P2];
        uchar **u;
        if (array_13 != NULL) {
            u = array_13;
        } else {
            u = array_23[P2];
        }
        ssot->P1(b1, u, n, data_size, v_out_13, count_band);
    }
    delete ssot;
}

void DPFORAM::PIW(uchar **array, const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_delta_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]PIW, index_23 = (%llu, %llu), n = %llu\n", this->n_, index_23[0], index_23[1], n);
    // print_bytes(v_delta_13, data_size, "v_delta_13");
    assert((n > 0) && "PIW n == 0");
    if (n == 1) {
        xor_bytes(array[0], v_delta_23[0], data_size, array[0]);
    } else if (n <= this->pseudo_dpf_threshold_) {
        PSEUDO_DPF_PIW(array, n, data_size, index_23, v_delta_23, count_band);
    } else {
        DPF_PIW(array, n, data_size, index_23, v_delta_23, count_band);
    }
}

void DPFORAM::DPF_PIW(uchar **array, const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_delta_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]PIW, index_23 = (%llu, %llu), n = %llu\n", this->n_, index_23[0], index_23[1], n);
    // print_bytes(v_delta_13, data_size, "v_delta_13");

    uint64_t log_n = uint64_log2(n);
    // fprintf(stderr, "[%llu]PIW, log_n = %llu\n", this->n_, log_n);

    uchar *query_23[2];
    uint query_size = this->fss_.Gen(index_23[0] ^ index_23[1], log_n, query_23);

    this->conn_[0]->Write(query_23[0], query_size, count_band);
    this->conn_[1]->Write(query_23[1], query_size, count_band);

    this->conn_[0]->Read(query_23[1], query_size);
    this->conn_[1]->Read(query_23[0], query_size);

    for (uint i = 0; i < 2; i++) {
        uchar dpf_out[n];
        this->fss_.EvalAll(query_23[i], log_n, dpf_out);
        // #pragma omp for
        for (uint64_t j = 0; j < n; j++) {
            // fprintf(stderr, "PIW i = %u, j = %llu, jj = %llu, dpf = %u\n", i, j, j ^ index_23[i], dpf_out[j ^ index_23[i]]);
            if (dpf_out[j ^ index_23[i]]) {
                xor_bytes(array[j], v_delta_23[i], data_size, array[j]);
            }
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] query_23[i];
    }
}

void DPFORAM::PSEUDO_DPF_PIW(uchar **array, const uint64_t n, const uint64_t data_size, const uint64_t index_23[2], uchar *v_delta_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]PIW, index_23 = (%llu, %llu), n = %llu\n", this->n_, index_23[0], index_23[1], n);
    // print_bytes(v_delta_13, data_size, "v_delta_13");

    uint64_t byte_length = uint64_ceil_divide(n, 8ULL);
    uchar *dpf_out[2];
    for (uint i = 0; i < 2; i++) {
        dpf_out[i] = new uchar[byte_length];
    }

    this->fss_.PseudoGen(&this->prgs_[0], index_23[0] ^ index_23[1], byte_length, dpf_out[1]);
    this->conn_[1]->Write(dpf_out[1], byte_length, count_band);
    this->conn_[0]->Read(dpf_out[1], byte_length);
    this->prgs_[1].GenerateBlock(dpf_out[0], byte_length);

    for (uint i = 0; i < 2; i++) {
        for (uint64_t j = 0; j < n; j++) {
            // fprintf(stderr, "PIW i = %u, j = %llu, jj = %llu, dpf = %u\n", i, j, j ^ index_23[i], dpf_out[j ^ index_23[i]]);
            if (this->fss_.PseudoEval(j ^ index_23[i], dpf_out[i])) {
                xor_bytes(array[j], v_delta_23[i], data_size, array[j]);
            }
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] dpf_out[i];
    }
}

void DPFORAM::GetLatestData(uchar *v_read_13,
                            uchar *v_cache_13, const bool is_cached_23[2],
                            uchar *v_out_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]GetLatestData, v_meta_13 = %d\n", this->n_, v_meta_13[0]);

    SSOT *ssot = new SSOT(this->party_, this->conn_, this->rnd_, this->prgs_);
    if (this->party_ == 2) {
        const uint P1 = 0, P0 = 1;

        this->conn_[P0]->Write(v_read_13, this->data_size_, count_band);
        this->conn_[P0]->Write(v_cache_13, this->data_size_, count_band);

        ssot->P2(2, this->data_size_, count_band);

        this->conn_[P0]->Read(v_out_23[P0], this->data_size_);
        this->conn_[P1]->Read(v_out_23[P1], this->data_size_);
    } else if (this->party_ == 0) {
        const uint P2 = 0, P1 = 1;
        uchar v_read_12[this->data_size_];
        uchar v_cache_12[this->data_size_];
        this->conn_[P2]->Read(v_read_12, this->data_size_);
        this->conn_[P2]->Read(v_cache_12, this->data_size_);
        xor_bytes(v_read_12, v_read_13, this->data_size_, v_read_12);
        xor_bytes(v_cache_12, v_cache_13, this->data_size_, v_cache_12);

        uchar *u[2] = {v_read_12, v_cache_12};
        const uint64_t b0 = is_cached_23[P1] ^ is_cached_23[P2];
        ssot->P0(b0, u, 2, this->data_size_, v_out_23[P2], count_band);

        this->prgs_[P1].GenerateBlock(v_out_23[P1], this->data_size_);
        xor_bytes(v_out_23[P2], v_out_23[P1], this->data_size_, v_out_23[P2]);
        this->conn_[P2]->Write(v_out_23[P2], this->data_size_, count_band);
    } else if (this->party_ == 1) {
        const uint P0 = 0, P2 = 1;
        uchar *v[2] = {v_read_13, v_cache_13};
        const uint64_t b1 = is_cached_23[P2];
        ssot->P1(b1, v, 2, this->data_size_, v_out_23[P2], count_band);

        this->prgs_[P0].GenerateBlock(v_out_23[P0], this->data_size_);
        this->conn_[P2]->Write(v_out_23[P2], this->data_size_, count_band);
    }
    delete ssot;
}

void DPFORAM::ShareTwoThird(const uchar *v_in_13, const uint64_t data_size, uchar *v_out_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]ShareTwoThird\n", this->n_);
    this->conn_[1]->Write(v_in_13, data_size, count_band);
    this->conn_[0]->Read(v_out_23[0], data_size);
    memcpy(v_out_23[1], v_in_13, data_size);
}

void DPFORAM::ShareIndexTwoThird(const uint64_t index_13, const uint64_t n, uint64_t index_23[2], bool count_band) {
    // fprintf(stderr, "ShareIndexTwoThird, index_13 = %llu, ", index_13);
    uint64_t rand_range = n;
    this->conn_[1]->WriteLong(index_13, count_band);
    index_23[0] = this->conn_[0]->ReadLong() % rand_range;
    index_23[1] = index_13 % rand_range;
}

void DPFORAM::ReadPositionMap(const uint64_t index_23[2], uint64_t cache_index_23[2], bool is_cached_23[2], bool read_only) {
    // fprintf(stderr, "[%llu]ReadPositionMap, index_23 = (%llu, %llu), read_only = %d\n", this->n_, index_23[0], index_23[1], read_only);

    uint64_t data_size = byte_length(this->n_ << 1);
    uint64_t data_per_block = 1ULL << this->tau_;
    // fprintf(stderr, "[%llu]ReadPositionMap, data_size = %llu, data_per_block = %llu\n", this->n_, data_size, data_per_block);

    uint64_t block_index_23[2];
    uint64_t data_index_23[2];
    uchar *old_block_23[2];
    for (uint i = 0; i < 2; i++) {
        block_index_23[i] = index_23[i] >> this->tau_;
        data_index_23[i] = index_23[i] & (data_per_block - 1);
        old_block_23[i] = new uchar[this->position_map_->data_size_];
    }

    // read block from array
    this->position_map_->Read(block_index_23, old_block_23, read_only);

    // read data from block
    uchar **old_data_array_23[2];
    uint64_t bytes_per_block = (this->position_map_->data_size_ + data_per_block - 1) / data_per_block;
    for (uint i = 0; i < 2; i++) {
        old_data_array_23[i] = new uchar *[data_per_block];
        for (uint j = 0; j < data_per_block; j++) {
            old_data_array_23[i][j] = &old_block_23[i][j * bytes_per_block];
        }
    }
    uchar old_data_13[data_size];
    PIR(old_data_array_23, data_per_block, data_size, data_index_23, old_data_13, !read_only);
    uchar *old_data_23[2];
    for (uint i = 0; i < 2; i++) {
        old_data_23[i] = new uchar[data_size];
    }
    this->ShareTwoThird(old_data_13, data_size, old_data_23, !read_only);
    for (uint i = 0; i < 2; i++) {
        cache_index_23[i] = bytes_to_uint64(old_data_23[i], data_size);
        is_cached_23[i] = cache_index_23[i] & 1;
        cache_index_23[i] = (cache_index_23[i] >> 1) % this->n_;
    }

    if (read_only == false) {
        uchar new_block_13[this->position_map_->data_size_];
        memcpy(new_block_13, old_block_23[0], this->position_map_->data_size_);

        uchar *data_array_13[data_per_block];
        for (uint i = 0; i < data_per_block; i++) {
            data_array_13[i] = &new_block_13[i * bytes_per_block];
        }
        uchar new_data_13[data_size];
        uint64_t new_cache_index = (this->cache_ctr_ << 1) + 1ULL;
        uint64_to_bytes(new_cache_index, new_data_13, data_size);

        for (uint i = 0; i < 2; i++) {
            xor_bytes(old_data_23[i], new_data_13, data_size, old_data_23[i]);
        }
        PIW(data_array_13, data_per_block, data_size, data_index_23, old_data_23, !read_only);

        uchar *new_block_23[2];
        for (uint i = 0; i < 2; i++) {
            new_block_23[i] = new uchar[this->position_map_->data_size_];
        }
        this->ShareTwoThird(new_block_13, this->position_map_->data_size_, new_block_23, !read_only);
        this->position_map_->Write(block_index_23, old_block_23, new_block_23, !read_only);

        for (uint i = 0; i < 2; i++) {
            delete[] new_block_23[i];
        }
    }

    for (uint i = 0; i < 2; i++) {
        delete[] old_data_23[i];
        delete[] old_block_23[i];
        delete[] old_data_array_23[i];
    }
}

void DPFORAM::Read(const uint64_t index_23[2], uchar *v_out_23[2], bool read_only) {
    // fprintf(stderr, "[%llu]Read, index_23 = (%llu, %llu)\n", this->n_, index_23[0], index_23[1]);

    if (this->n_ == 1) {
        for (uint i = 0; i < 2; i++) {
            this->ShareTwoThird(this->write_array_13_[0], this->data_size_, v_out_23, !read_only);
        }
    } else if (this->n_ >= this->ssot_threshold_) {
        SSOT_Read(index_23, v_out_23, read_only);
    } else {
        DPF_Read(index_23, v_out_23, read_only);
    }
}

void DPFORAM::DPF_Read(const uint64_t index_23[2], uchar *v_out_23[2], bool read_only) {
    // fprintf(stderr, "[%llu]Read, index_23 = (%llu, %llu)\n", this->n_, index_23[0], index_23[1]);

    uchar v_read_13[this->data_size_];
    PIR(this->read_array_23_, this->n_, this->data_size_, index_23, v_read_13, !read_only);

    uint64_t cache_index_23[2];
    bool is_cached_23[2];
    if (this->position_map_ != NULL) {
        ReadPositionMap(index_23, cache_index_23, is_cached_23, read_only);
    }

    uchar v_cache_13[this->data_size_];
    PIR(this->cache_23_, this->n_, this->data_size_, cache_index_23, v_cache_13, !read_only);
    GetLatestData(v_read_13, v_cache_13, is_cached_23, v_out_23, !read_only);
}

void DPFORAM::SSOT_Read(const uint64_t index_23[2], uchar *v_out_23[2], bool read_only) {
    // fprintf(stderr, "[%llu]Read, index_23 = (%llu, %llu)\n", this->n_, index_23[0], index_23[1]);

    uchar v_read_13[this->data_size_];
    SSOT_PIR(this->write_array_13_, NULL, this->n_, this->data_size_, index_23, v_read_13, !read_only);
    this->ShareTwoThird(v_read_13, this->data_size_, v_out_23, !read_only);
}

void DPFORAM::Write(const uint64_t index_23[2], uchar *old_data_23[2], uchar *new_data_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]Write, index_23 = (%llu, %llu)\n", this->n_, index_23[0], index_23[1]);

    if (this->n_ == 1) {
        memcpy(this->write_array_13_[0], new_data_23[0], this->data_size_);
    } else if (this->n_ >= this->ssot_threshold_) {
        SSOT_Write(index_23, old_data_23, new_data_23, count_band);
    } else {
        DPF_Write(index_23, old_data_23, new_data_23, count_band);
    }
}

void DPFORAM::DPF_Write(const uint64_t index_23[2], uchar *old_data_23[2], uchar *new_data_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]Write, index_23 = (%llu, %llu)\n", this->n_, index_23[0], index_23[1]);

    uchar *delta_data_23[2];
    for (uint i = 0; i < 2; i++) {
        delta_data_23[i] = new uchar[this->data_size_];
        xor_bytes(old_data_23[i], new_data_23[i], this->data_size_, delta_data_23[i]);
    }

    PIW(this->write_array_13_, this->n_, this->data_size_, index_23, delta_data_23, count_band);
    this->AppendCache(new_data_23, count_band);

    for (uint i = 0; i < 2; i++) {
        delete[] delta_data_23[i];
    }
}

void DPFORAM::SSOT_Write(const uint64_t index_23[2], uchar *old_data_23[2], uchar *new_data_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]Write, index_23 = (%llu, %llu)\n", this->n_, index_23[0], index_23[1]);
    uchar *delta_data_23[2];
    for (uint i = 0; i < 2; i++) {
        delta_data_23[i] = new uchar[this->data_size_];
        xor_bytes(old_data_23[i], new_data_23[i], this->data_size_, delta_data_23[i]);
    }

    PIW(this->write_array_13_, this->n_, this->data_size_, index_23, delta_data_23, count_band);

    for (uint i = 0; i < 2; i++) {
        delete[] delta_data_23[i];
    }
}

void DPFORAM::AppendCache(uchar *v_new_23[2], bool count_band) {
    // fprintf(stderr, "[%llu]AppendCache, this->cache_ctr_ = %llu\n", this->n_, this->cache_ctr_);
    for (uint i = 0; i < 2; i++) {
        memcpy(this->cache_23_[i][this->cache_ctr_], v_new_23[i], this->data_size_);
    }
    this->cache_ctr_++;
    if (this->cache_ctr_ == this->n_) {
        this->Flush();
        if (this->position_map_ != NULL) {
            this->position_map_->Reset();
        }
    }
}

void DPFORAM::Flush(bool count_band) {
    // fprintf(stderr, "[%llu]Flush\n", this->n_);
    uchar *array_23[2];
    for (uint64_t j = 0; j < this->n_; j++) {
        for (uint i = 0; i < 2; i++) {
            array_23[i] = this->read_array_23_[i][j];
        }
        this->ShareTwoThird(this->write_array_13_[j], this->data_size_, array_23, count_band);
    }
    this->cache_ctr_ = 0;
}

void DPFORAM::PrintMetadata() {
    fprintf(stderr, "========== PrintMetadata ==========\n");
    fprintf(stderr, "party_: %u\n", this->party_);
    fprintf(stderr, "tau_: %llu\n", this->tau_);
    fprintf(stderr, "n_: %llu\n", this->n_);
    fprintf(stderr, "data_size_: %llu\n", this->data_size_);
    fprintf(stderr, "cache_ctr_: %llu\n", this->cache_ctr_);
    fprintf(stderr, "position_map_: %d\n", (this->position_map_ != NULL));
    for (uint i = 0; i < this->n_; i++) {
        print_bytes(this->read_array_23_[0][i], this->data_size_, "this->read_array_23_[0]", i);
    }
    for (uint i = 0; i < this->n_; i++) {
        print_bytes(this->read_array_23_[1][i], this->data_size_, "this->read_array_23_[1]", i);
    }
    for (uint i = 0; i < this->n_; i++) {
        print_bytes(this->write_array_13_[i], this->data_size_, "this->write_array_13_", i);
    }
    for (uint i = 0; i < this->n_; i++) {
        print_bytes(this->cache_23_[0][i], this->data_size_, "this->cache_23_[0]", i);
    }
    for (uint i = 0; i < this->n_; i++) {
        print_bytes(this->cache_23_[1][i], this->data_size_, "this->cache_23_[1]", i);
    }
    fprintf(stderr, "========== PrintMetadata ==========\n");
}

void DPFORAM::Test(uint iterations) {
    fprintf(stderr, "Test, iterations = %u \n", iterations);
    uint64_t party_time = 0;
    uint64_t time;

    for (uint iteration = 0; iteration < iterations; iteration++) {
        // fprintf(stderr, "Test, iteration = %u\n", iteration);
        uint64_t rand_range = this->n_ - 1;
        uint64_t index_13 = rand_uint64() % rand_range;
        uint64_t index_23[2];
        this->ShareIndexTwoThird(index_13, this->n_, index_23, false);
        // fprintf(stderr, "Test, rand_range = %llu, index_13 = %llu, index_23 = (%llu, %llu)\n", rand_range, index_13, index_23[0], index_23[1]);

        // fprintf(stderr, "\nTest, ========== Read old data ==========\n");

        uchar *old_data_23[2];
        for (uint i = 0; i < 2; i++) {
            old_data_23[i] = new uchar[this->data_size_];
        }
        time = timestamp();
        this->Read(index_23, old_data_23);
        party_time += timestamp() - time;

        // fprintf(stderr, "\nTest, ========== Write random data ==========\n");
        uchar *new_data_23[2];
        for (uint i = 0; i < 2; i++) {
            new_data_23[i] = new uchar[this->data_size_];
            this->prgs_[i].GenerateBlock(new_data_23[i], this->data_size_);
        }

        time = timestamp();
        this->Write(index_23, old_data_23, new_data_23);
        party_time += timestamp() - time;

        // fprintf(stderr, "\nTest, ========== Read validation data ==========\n");
        uchar *verify_data_23[2];
        for (uint i = 0; i < 2; i++) {
            verify_data_23[i] = new uchar[this->data_size_];
        }

        this->Read(index_23, verify_data_23, true);

        this->conn_[1]->Write(verify_data_23[0], this->data_size_, false);
        uchar verify_data[this->data_size_];
        this->conn_[0]->Read(verify_data, this->data_size_);
        xor_bytes(verify_data_23[0], verify_data_23[1], verify_data, this->data_size_, verify_data);

        uchar new_data[this->data_size_];
        this->conn_[1]->Write(new_data_23[0], this->data_size_, false);
        this->conn_[0]->Read(new_data, this->data_size_);
        xor_bytes(new_data, new_data_23[0], new_data_23[1], this->data_size_, new_data);

        if (memcmp(verify_data, new_data, this->data_size_) == 0) {
            // fprintf(stderr, "%u, Pass\n", iteration);
        } else {
            fprintf(stderr, "%u, Fail !!!\n", iteration);
            exit(1);
        }

        for (uint i = 0; i < 2; i++) {
            delete[] old_data_23[i];
            delete[] verify_data_23[i];
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
    fprintf(stderr, "n = %llu\n", this->n_);
    fprintf(stderr, "Party Bandwidth(byte): %llu\n", party_bandwidth / iterations);
    fprintf(stderr, "Party execution time(microsec): %llu\n", party_time / iterations);
    fprintf(stderr, "Total Bandwidth(byte): %llu\n", total_bandwidth / iterations);
    fprintf(stderr, "Max Execution time(microsec): %llu\n", max_time / iterations);
    fprintf(stderr, "\n");
}
