#ifndef BULK_DATA_H_
#define BULK_DATA_H_

#include <inttypes.h>

#include "constant.h"
#include "prg.h"
#include "typedef.h"
#include "util.h"

template <typename D>
class BulkData {
private:
    static inline PRG *prg_ = new PRG();

public:
    std::vector<D> data_;

    friend BulkData operator+(BulkData lhs, const BulkData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend BulkData operator-(BulkData lhs, const BulkData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    uint Size() {
        if (this->data_.size() == 0) {
            return 0;
        } else {
            return this->data_[0].Size() * DATA_PER_BLOCK;
        }
    }
    bool IsSymmetric() {
        if (this->data_.size() == 0) {
            return true;
        } else {
            return this->data_[0].IsSymmetric();
        }
    }
    std::vector<D> Data() { return this->data_; }

    BulkData() {
        this->data_.resize(DATA_PER_BLOCK);
    }

    BulkData(const uint size) {
        this->data_.resize(DATA_PER_BLOCK);
        this->Resize(size);
        this->Reset();
    }

    BulkData(const BulkData &other) {
        this->data_ = other.data_;
    }

    ~BulkData() {
        // this->data_.clear();
    }

    BulkData<D> &operator=(const BulkData &other) {
        // copy operation
        if (this == &other) return *this;
        this->data_ = other.data_;
        return *this;
    }

    BulkData<D> operator-() {
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i] = -this->data_[i];
        }
        return *this;
    }

    BulkData<D> &operator+=(const BulkData &rhs) {
        assert(this->data_.size() == rhs.data_.size());
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i] += rhs.data_[i];
        }
        return *this;
    }

    BulkData<D> &operator-=(const BulkData &rhs) {
        assert(this->data_.size() == rhs.data_.size());
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i] -= rhs.data_[i];
        }
        return *this;
    }

    bool operator==(const BulkData &rhs) {
        if (this->data_.size() != rhs.data_.size()) return false;
        for (uint i = 0; i < this->data_.size(); i++) {
            if (this->data_[i] != rhs.data_[i]) return false;
        }
        return true;
    }

    void Dump(std::vector<uchar> &data) {
        data.resize(this->Size());
        std::vector<uchar> dump;
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i].Dump(dump);
            memcpy(data.data() + i * dump.size(), dump.data(), dump.size());
            // data.insert(data.end(), dump.begin(), dump.end());
        }
    }

    void Load(std::vector<uchar> &data) {
        uint data_size = data.size() / DATA_PER_BLOCK;
        std::vector<uchar> data_item(data_size);
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            memcpy(data_item.data(), data.data() + data_size * i, data_size);
            // std::vector<uchar> data_item(data.begin() + data_size * i, data.begin() + data_size * (i + 1));
            this->data_[i].Load(data_item);
        }
    }

    void Reset() {
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i].Reset();
        }
    }

    void Resize(const uint size) {
        uint data_size = size / DATA_PER_BLOCK;
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            this->data_[i].Resize(data_size);
        }
    }

    void Random(PRG *prg = NULL) {
        if (prg == NULL) prg = this->prg_;
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i].Random(prg);
        }
    }

    void Print(const char *title = "") {
#ifdef DEBUG
        if (strlen(title) > 0) {
            debug_print("%s\n", title);
        }
        for (uint i = 0; i < this->data_.size(); i++) {
            debug_print("%u ", i);
            this->data_[i].Print();
        }
        debug_print("\n");
#endif
    }
};

#endif /* BULK_DATA_H_ */