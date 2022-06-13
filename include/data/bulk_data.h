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
        // assert(this->data_.size() == rhs.data_.size());
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i] += rhs.data_[i];
        }
        return *this;
    }

    BulkData<D> &operator-=(const BulkData &rhs) {
        // assert(this->data_.size() == rhs.data_.size());
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i] -= rhs.data_[i];
        }
        return *this;
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

    void DumpBuffer(uchar *buffer) {
        uint data_size = this->data_[0].Size();
        for (uint i = 0; i < this->data_.size(); i++) {
            this->data_[i].DumpBuffer(buffer + i * data_size);
        }
    }

    std::vector<uchar> DumpVector() {
        std::vector<uchar> data(this->Size());
        DumpBuffer(data.data());
        return data;
    }

    void LoadBuffer(uchar *buffer) {
        uint data_size = this->data_[0].Size();
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            this->data_[i].LoadBuffer(buffer + i * data_size);
        }
    }

    void LoadVector(std::vector<uchar> &data) {
        LoadBuffer(data.data());
    }

    std::vector<D> GetData() {
        return this->data_;
    }

    void SetData(std::vector<D> &data) {
        this->data_ = data;
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
    BulkData() {
    }

    BulkData(const uint size) {
        this->Resize(size);
        this->Reset();
    }

    BulkData(const BulkData &other) {
        this->data_ = other.data_;
    }

    ~BulkData() {}

    BulkData<BinaryData> &operator=(const BulkData<BinaryData> &other) {
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

    void DumpBuffer(uchar *buffer) {
        this->data_.DumpBuffer(buffer);
    }

    std::vector<uchar> DumpVector() {
        return this->data_.DumpVector();
    }

    void LoadBuffer(uchar *buffer) {
        this->data_.LoadBuffer(buffer);
    }

    void LoadVector(std::vector<uchar> &data) {
        this->data_.LoadVector(data);
    }

    std::vector<BinaryData> GetData() {
        uint data_size = this->data_.Size() / DATA_PER_BLOCK;
        std::vector<uchar> dump = this->data_.DumpVector();
        std::vector<BinaryData> data(DATA_PER_BLOCK, BinaryData(data_size));
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            data[i].LoadBuffer(dump.data() + i * data_size);
        }
        return data;
    }

    void SetData(std::vector<BinaryData> &data) {
        assert(data.size() == DATA_PER_BLOCK);
        uint data_size = data[0].Size();
        std::vector<uchar> buffer(data_size * DATA_PER_BLOCK);
        for (uint i = 0; i < DATA_PER_BLOCK; i++) {
            data[i].DumpBuffer(buffer.data() + i * data_size);
        }
        this->data_.LoadBuffer(buffer.data());
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