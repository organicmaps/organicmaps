#ifndef SZ3_LINEAR_QUANTIZER_HPP
#define SZ3_LINEAR_QUANTIZER_HPP

#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "SZ3/def.hpp"
#include "SZ3/quantizer/Quantizer.hpp"
#include "SZ3/utils/MemoryUtil.hpp"

namespace SZ3 {
template <class T>
class LinearQuantizer : public concepts::QuantizerInterface<T, int> {
    static_assert(std::is_floating_point<T>::value, "LinearQuantizer only takes a floating-point T");

public:
    LinearQuantizer()
        : error_bound(1),
          error_bound_reciprocal(1),
          radius(32768) {
    }

    LinearQuantizer(double eb, int r = 32768)
        : error_bound(eb),
          error_bound_reciprocal(1.0 / eb),
          radius(r) {
        assert(eb != 0);
    }

    double get_eb() const { return error_bound; }

    void set_eb(double eb) {
        error_bound = eb;
        error_bound_reciprocal = 1.0 / eb;
    }

    std::pair<int, int> get_out_range() const override { return std::make_pair(0, radius * 2); }

    ALWAYS_INLINE int quantize_and_overwrite(T& data, T pred) override {
        T diff = data - pred;
        // NaN data makes this product NaN and infinities push it past the int64_t range; casting either is
        // undefined, so the range test is in floating point. What is not representable falls through to unpred.
        double scaled = fabs(diff) * this->error_bound_reciprocal;
        if (scaled < this->radius * 2 - 1) {
            int64_t quant_index = static_cast<int64_t>(scaled) + 1;
            quant_index >>= 1;
            int half_index = quant_index;
            int quant_index_shifted;
            if (diff < 0) {
                quant_index_shifted = this->radius - half_index;
            } else {
                quant_index_shifted = this->radius + half_index;
            }
            T decompressed_data = recover_pred(pred, quant_index_shifted);
            double err = fabs(static_cast<double>(decompressed_data) - data);
            if (err <= this->error_bound) {
                data = decompressed_data;
                return quant_index_shifted;
            }
        }
        unpred.push_back(data);
        return 0;
    }

    // recover the data using the quantization index
    ALWAYS_INLINE T recover(T pred, int quant_index) override {
        if (quant_index) {
            return recover_pred(pred, quant_index);
        } else {
            return recover_unpred();
        }
    }

    ALWAYS_INLINE T recover_pred(T pred, int quant_index) {
        // quant_index comes from the stream; in int, 2 * (quant_index - radius) overflows past INT_MAX/2.
        // Exact for every index a valid stream carries.
        return pred + nofma(2 * (static_cast<int64_t>(quant_index) - this->radius) * this->error_bound);
    }

    ALWAYS_INLINE T recover_unpred() {
        if (index >= unpred.size()) throw std::out_of_range("SZ3: ran out of unpredictable values while decompressing");
        return unpred[index++];
    }

    ALWAYS_INLINE int force_save_unpred(T ori) override {
        unpred.push_back(ori);
        return 0;
    }

    size_t size_est() {
        return sizeof(uid) + sizeof(this->error_bound) + sizeof(this->radius) + sizeof(uint64_t) +
               unpred.size() * sizeof(T);
    }

    void save(unsigned char*& c) const override {
        write(uid, c);
        write(this->error_bound, c);
        write(this->radius, c);
        size_t unpred_size = unpred.size();
        write<uint64_t>(unpred_size, c);
        if (unpred_size > 0) {
            write(unpred.data(), unpred.size(), c);
        }
    }

    void load(const unsigned char*& c, size_t& remaining_length) override {
        uchar uid_read;
        read(uid_read, c, remaining_length);
        if (uid_read != uid) {
            throw std::invalid_argument("LinearQuantizer uid mismatch");
        }
        read(this->error_bound, c, remaining_length);
        this->error_bound_reciprocal = 1.0 / this->error_bound;
        read(this->radius, c, remaining_length);
        uint64_t unpred_size = 0;
        read(unpred_size, c, remaining_length);
        if (unpred_size > 0) {
            // resize() below is sized from the stream, so check the count against the bytes that exist first.
            if (unpred_size > remaining_length / sizeof(T))
                throw std::out_of_range("SZ3: unpredictable value count exceeds the compressed buffer");
            unpred.resize(unpred_size);
            read(unpred.data(), unpred_size, c, remaining_length);
        }
        index = 0;
    }

    void print() override {
        printf("[LinearQuantizer] error_bound = %.8G, radius = %d, unpred = %zu\n", error_bound, radius, unpred.size());
    }

private:
    std::vector<T> unpred;
    size_t index = 0; // used in decompression only
    uchar uid = 0b10;

    double error_bound;
    double error_bound_reciprocal;
    int radius; // quantization interval radius
};
} // namespace SZ3
#endif
