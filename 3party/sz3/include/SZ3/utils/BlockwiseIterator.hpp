#ifndef SZ3_BLOCKWISE_ITERATOR_HPP
#define SZ3_BLOCKWISE_ITERATOR_HPP

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "SZ3/def.hpp"

namespace SZ3 {

/**
 * @brief A class representing block data with N dimensions.
 *
 * This class provides functionality for managing multidimensional data blocks,
 * including iterating over blocks, copying data in and out, and handling padding.
 *
 * @tparam T The type of the data elements.
 * @tparam N The number of dimensions.
 */
template <class T, uint N>
class block_data : public std::enable_shared_from_this<block_data<T, N>> {
   public:
    /**
     * @brief A nested class for iterating over blocks of data.
     */
    class block_iterator {
       public:
        /**
         * @brief Constructs a block iterator.
         *
         * @param mddata_ A shared pointer to the block data.
         * @param block_size_ The size of each block.
         */
        block_iterator(std::shared_ptr<block_data> &&mddata_, size_t block_size_) noexcept
            : mddata(mddata_), block_size(block_size_) {
            std::fill(offset.begin(), offset.end(), 0);
        }

        /**
         * @brief Advances the iterator to the next block.
         *
         * @return true if the iterator successfully moved to the next block, false otherwise.
         */
        ALWAYS_INLINE bool next() {
            size_t i = N - 1;
            offset[i] += block_size;
            while (i && (offset[i] >= mddata->dims[i])) {
                offset[i] = 0;
                offset[--i] += block_size;
            }
            return (offset[0] < mddata->dims[0]);
        }

        /**
         * @brief Gets the range of the current block in each dimension.
         *
         * @return An array of pairs representing the [start, end) indices of the block in each dimension.
         */
        ALWAYS_INLINE std::array<std::pair<size_t, size_t>, N> get_block_range() const {
            std::array<std::pair<size_t, size_t>, N> range;
            for (uint i = 0; i < N; i++) {
                range[i].first = offset[i];
                range[i].second = std::min(offset[i] + block_size, mddata->dims[i]);
            }
            return range;
        }

        /**
         * @brief Gets a pointer to the data of the current block.
         *
         * @param args The relative indices within the block.
         * @return A pointer to the data at the specified indices.
         */
        template <class... Idx>
        ALWAYS_INLINE T *get_block_data(Idx... args) const {
            auto strides = get_dim_strides();
            auto idx = std::array<size_t, N>{static_cast<size_t>(std::forward<Idx>(args))...};
            size_t off = 0;
            for (uint i = 0; i < N; i++) {
                off += (idx[i] + offset[i]) * strides[i];
            }
            return mddata->dataptr() + off;
        }

        /**
         * @brief Gets the strides for each dimension, including padding.
         *
         * @return An array of strides for each dimension.
         */
        ALWAYS_INLINE std::array<size_t, N> get_dim_strides() const { return mddata->ds_padding; }

        /**
         * @brief Iterates over all elements in the current block and applies a function.
         *
         * @tparam Func The type of the function to apply.
         * @param block The block iterator.
         * @param func The function to apply to each element.
         */
        template <typename Func>
        static ALWAYS_INLINE void foreach (const block_iterator &block, Func && func) {
            auto range = block.get_block_range();
            if constexpr (N == 1) {
                T *d = block.get_block_data(0);
                for (size_t i = 0; i < range[0].second - range[0].first; i++) {
                    func(d++, {i});
                }
            } else if constexpr (N == 2) {
                for (size_t i = 0; i < range[0].second - range[0].first; i++) {
                    T *d = block.get_block_data(i, 0);
                    for (size_t j = 0; j < range[1].second - range[1].first; j++) {
                        func(d++, {i, j});
                    }
                }
            } else if constexpr (N == 3) {
                for (size_t i = 0; i < range[0].second - range[0].first; i++) {
                    for (size_t j = 0; j < range[1].second - range[1].first; j++) {
                        T *d = block.get_block_data(i, j, 0);
                        for (size_t k = 0; k < range[2].second - range[2].first; k++) {
                            func(d++, {i, j, k});
                        }
                    }
                }
            } else if constexpr (N == 4) {
                for (size_t i = 0; i < range[0].second - range[0].first; i++) {
                    for (size_t j = 0; j < range[1].second - range[1].first; j++) {
                        for (size_t k = 0; k < range[2].second - range[2].first; k++) {
                            T *d = block.get_block_data(i, j, k, 0);
                            for (size_t t = 0; t < range[3].second - range[3].first; t++) {
                                func(d++, {i, j, k, t});
                            }
                        }
                    }
                }
            } else {
                throw std::invalid_argument("N (dimension) should be less than 5");
            }
        }

        /**
         * @brief Iterates over sampled elements in the current block and applies a function.
         *
         * @tparam Func The type of the function to apply.
         * @param block The block iterator.
         * @param func The function to apply to sampled elements.
         */
        template <typename Func>
        static ALWAYS_INLINE void foreach_sampling(const block_iterator &block, Func &&func) {
            size_t min_size = std::numeric_limits<size_t>::max();
            for (const auto &r : block.get_block_range()) {
                min_size = std::min(min_size, r.second - r.first);
            }
            if constexpr (N == 1) {
                func(block.get_block_data(0), {0});
                func(block.get_block_data(min_size - 1), {min_size - 1});
            } else if constexpr (N <= 4) {
                for (size_t i = 0; i < min_size; i++) {
                    size_t j = min_size - 1 - i;
                    if constexpr (N == 2) {
                        func(block.get_block_data(i, i), {i, i});
                        func(block.get_block_data(i, j), {i, j});
                    } else if constexpr (N == 3) {
                        func(block.get_block_data(i, i, i), {i, i, i});
                        func(block.get_block_data(i, i, j), {i, i, j});
                        func(block.get_block_data(i, j, i), {i, j, i});
                        func(block.get_block_data(i, j, j), {i, j, j});
                    } else if constexpr (N == 4) {
                        func(block.get_block_data(i, i, i, i), {i, i, i, i});
                        func(block.get_block_data(i, i, i, j), {i, i, i, j});
                        func(block.get_block_data(i, i, j, i), {i, i, j, i});
                        func(block.get_block_data(i, i, j, j), {i, i, j, j});
                        func(block.get_block_data(i, j, i, i), {i, j, i, i});
                        func(block.get_block_data(i, j, i, j), {i, j, i, j});
                        func(block.get_block_data(i, j, j, i), {i, j, j, i});
                        func(block.get_block_data(i, j, j, j), {i, j, j, j});
                    }
                }
            } else {
                throw std::invalid_argument("N (dimension) should be less than 5");
            }
        }

        friend block_data;
        std::shared_ptr<block_data> mddata;

       private:
        std::array<size_t, N> offset;
        size_t block_size;
    };

    ~block_data() {
        if (padding > 0 && !internal_buffer.empty() && data_cp_dst != nullptr) {
            copy_data_with_padding(data_cp_dst, ds, data_padding, ds_padding, dims);
        }
    }

    block_data(T *data_, const std::vector<size_t> &dims_, size_t padding_ = 0, bool data_valid = true)
        : padding(padding_) {
        if (dims_.size() != N) {
            throw std::invalid_argument("#dims does not match!");
        }
        std::copy_n(dims_.begin(), N, dims.begin());
        cal_dim_strides();

        if (padding > 0) {
            internal_buffer.resize(num_padding);
            size_t offset = std::accumulate(ds_padding.begin(), ds_padding.end(), static_cast<size_t>(0));
            data_padding = &internal_buffer[padding * offset];
            if (data_valid) {
                copy_data_with_padding(data_padding, ds_padding, data_, ds, dims);
            } else {
                data_cp_dst = data_;
            }
        } else {
            data_padding = data_;
        }
    }

    /**
     * @brief The block's `num` values in unpadded layout. With padding this is an internal copy,
     *        materialized on the first call and valid until this object is destroyed.
     */
    const T *values() {
        if (padding == 0 || internal_buffer.empty()) {
            return data_padding;
        }
        unpadded_buffer.resize(num);
        copy_data_with_padding(unpadded_buffer.data(), ds, data_padding, ds_padding, dims);
        return unpadded_buffer.data();
    }

    block_iterator block_iter(size_t block_size) { return block_iterator(this->shared_from_this(), block_size); }

   protected:
    ALWAYS_INLINE T *dataptr() { return data_padding; }

   private:
    ALWAYS_INLINE void cal_dim_strides() {
        size_t cur_stride = 1, cur_stride_pading = 1;
        for (int i = N - 1; i >= 0; i--) {
            ds[i] = cur_stride;
            ds_padding[i] = cur_stride_pading;
            cur_stride *= dims[i];
            cur_stride_pading *= (dims[i] + padding);
        }
        num = cur_stride;
        num_padding = cur_stride_pading;
    }

    void copy_data_with_padding(T *dst, const std::array<size_t, N> &dst_stride, const T *src,
                                const std::array<size_t, N> &src_stride, const std::array<size_t, N> &dims_) {
        if (dst == nullptr || src == nullptr) {
            throw std::invalid_argument("Null pointer passed to copy_data_with_padding");
            return;
        }
        if constexpr (N == 1) {
            memcpy(&dst[0], &src[0], dims_[0] * sizeof(T));
        } else if constexpr (N == 2) {
            for (size_t i = 0; i < dims_[0]; i++) {
                memcpy(&dst[i * dst_stride[0]], &src[i * src_stride[0]], dims_[1] * sizeof(T));
            }
        } else if constexpr (N == 3) {
            for (size_t i = 0; i < dims_[0]; i++) {
                for (size_t j = 0; j < dims_[1]; j++) {
                    memcpy(&dst[i * dst_stride[0] + j * dst_stride[1]], &src[i * src_stride[0] + j * src_stride[1]],
                           dims_[2] * sizeof(T));
                }
            }
        } else if constexpr (N == 4) {
            for (size_t i = 0; i < dims_[0]; i++) {
                for (size_t j = 0; j < dims_[1]; j++) {
                    for (size_t k = 0; k < dims_[2]; k++) {
                        memcpy(&dst[i * dst_stride[0] + j * dst_stride[1] + k * dst_stride[2]],
                               &src[i * src_stride[0] + j * src_stride[1] + k * src_stride[2]], dims_[3] * sizeof(T));
                    }
                }
            }
        } else {
            throw std::invalid_argument("N (dimension) should be less than 5");
        }
    }

    std::array<size_t, N> dims;            // dimension
    std::array<size_t, N> ds, ds_padding;  // stride
    std::vector<T> internal_buffer;
    std::vector<T> unpadded_buffer;  // materialized on demand by values()
    T *data_cp_dst = nullptr;
    T *data_padding;  // point to either data_ or internal_buffer depending on padding
    size_t padding;
    size_t num, num_padding;
};

template <class T, uint N, typename Func>
ALWAYS_INLINE void foreach (T *data, size_t offset, const std::array<size_t, N> &begins,
                            const std::array<size_t, N> &ends, const std::array<size_t, N> &strides,
                            const std::array<size_t, N> &dim_offsets, Func && func) {
    if constexpr (N == 1) {
        for (size_t i = begins[0]; i < ends[0]; i += strides[0]) {
            T *d = data + offset + i * dim_offsets[0];
            func(d);
        }
    } else if constexpr (N == 2) {
        for (size_t i = begins[0]; i < ends[0]; i += strides[0]) {
            for (size_t j = begins[1]; j < ends[1]; j += strides[1]) {
                T *d = data + offset + i * dim_offsets[0] + j * dim_offsets[1];
                func(d);
            }
        }
    } else if constexpr (N == 3) {
        for (size_t i = begins[0]; i < ends[0]; i += strides[0]) {
            for (size_t j = begins[1]; j < ends[1]; j += strides[1]) {
                for (size_t k = begins[2]; k < ends[2]; k += strides[2]) {
                    T *d = data + offset + i * dim_offsets[0] + j * dim_offsets[1] + k * dim_offsets[2];
                    func(d);
                }
            }
        }
    } else if constexpr (N == 4) {
        for (size_t i = begins[0]; i < ends[0]; i += strides[0]) {
            for (size_t j = begins[1]; j < ends[1]; j += strides[1]) {
                for (size_t k = begins[2]; k < ends[2]; k += strides[2]) {
                    for (size_t l = begins[3]; l < ends[3]; l += strides[3]) {
                        T *d = data + offset + i * dim_offsets[0] + j * dim_offsets[1] + k * dim_offsets[2] +
                               l * dim_offsets[3];
                        func(d);
                    }
                }
            }
        }
    } else {
        throw std::invalid_argument("N (dimension) should be less than 5");
    }
}

}  // namespace SZ3
#endif
