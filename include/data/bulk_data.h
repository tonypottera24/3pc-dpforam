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
    std::vector<D> data_;

public:
    std::vector<D> GetData() {
        return this->data_;
    }

    void SetData(std::vector<D> &data) {
        this->data_ = data;
    }

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
    static bool IsSymmetric() {
        return D::IsSymmetric();
    }

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

    bool operator==(const BulkData &rhs) {
        if (this->data_.size() != rhs.data_.size()) return false;
        for (uint i = 0; i < this->data_.size(); i++) {
            if (this->data_[i] != rhs.data_[i]) return false;
        }
        return true;
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

    std::vector<uchar> Dump() {
        std::vector<uchar> data;
        std::vector<uchar> dump;
        for (uint i = 0; i < this->data_.size(); i++) {
            dump = this->data_[i].Dump();
            data.insert(data.end(), dump.begin(), dump.end());
        }
        return data;
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

template <>
class BulkData<BinaryData> {
private:
    static inline PRG *prg_ = new PRG();
    BinaryData data_;

public:
    friend BulkData operator+(BulkData lhs, const BulkData &rhs) {
        lhs += rhs;
        return lhs;
    }
    friend BulkData operator-(BulkData lhs, const BulkData &rhs) {
        lhs -= rhs;
        return lhs;
    }

    uint Size() {
        return this->data_.Size();
    }

    static bool IsSymmetric() {
        return true;
    }

    std::vector<BinaryData> GetData() {
        uint data_size = this->data_.Size() / DATA_PER_BLOCK;
        std::vector<uchar> dump = this->data_.Dump();
        std::vector<BinaryData> data(DATA_PER_BLOCK, BinaryData(data_size));
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            std::vector<uchar> buffer(dump.begin() + i * data_size, dump.begin() + (i + 1) * data_size);
            data[i].Load(buffer);
        }
        return data;
    }

    void SetData(std::vector<BinaryData> &data) {
        assert(data.size() == DATA_PER_BLOCK);
        std::vector<uchar> buffer;
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            std::vector<uchar> data_dump = data[i].Dump();
            buffer.insert(buffer.end(), data_dump.begin(), data_dump.end());
        }
        this->data_.Load(buffer);
    }

    BulkData() {
    }

    BulkData(const uint size) {
        this->Resize(size);
        this->Reset();
    }

    BulkData(const BulkData &other) {
        this->data_ = other.data_;
    }

    ~BulkData() {
        // this->data_.clear();
    }

    BulkData<BinaryData> &operator=(const BulkData<BinaryData> &other) {
        // copy operation
        if (this == &other) return *this;
        this->data_ = other.data_;
        return *this;
    }

    BulkData<BinaryData> operator-() {
        this->data_ = -this->data_;
        return *this;
    }

    bool operator==(const BulkData<BinaryData> &rhs) {
        return this->data_ == rhs.data_;
    }

    BulkData<BinaryData> &operator+=(const BulkData<BinaryData> &rhs) {
        this->data_ += rhs.data_;
        return *this;
    }

    BulkData<BinaryData> &operator-=(const BulkData<BinaryData> &rhs) {
        this->data_ -= rhs.data_;
        return *this;
    }

    std::vector<uchar> Dump() {
        return this->data_.Dump();
    }

    void Load(std::vector<uchar> &data) {
        this->data_.Load(data);
    }

    void Reset() {
        this->data_.Reset();
    }

    void Resize(const uint size) {
        this->data_.Resize(size);
    }

    void Random(PRG *prg = NULL) {
        this->data_.Random(prg);
    }

    void Print(const char *title = "") {
#ifdef DEBUG
        this->data_.Print(title);
#endif
    }
};

#endif /* BULK_DATA_H_ */