#ifndef SZ3_ITERATOR_HPP
#define SZ3_ITERATOR_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "SZ3/def.hpp"

namespace SZ3 {
// N-dimensional multi_dimensional_range
template <class T, uint N>
class multi_dimensional_range : public std::enable_shared_from_this<multi_dimensional_range<T, N>> {
   public:
    class multi_dimensional_iterator {
       public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = T const &;
        using pointer = T *;
        using iterator_category = std::bidirectional_iterator_tag;

        ~multi_dimensional_iterator() = default;

        multi_dimensional_iterator() = default;

        multi_dimensional_iterator(multi_dimensional_iterator const &) = default;

        multi_dimensional_iterator &operator=(multi_dimensional_iterator const &) = default;

        multi_dimensional_iterator(multi_dimensional_iterator &&) noexcept = default;

        multi_dimensional_iterator &operator=(multi_dimensional_iterator &&) noexcept = default;

        multi_dimensional_iterator(std::shared_ptr<multi_dimensional_range> &&range_,
                                   std::size_t current_offset_) noexcept
            : range(range_), local_index{}, global_offset(current_offset_) {}

        multi_dimensional_iterator &operator--() {
            size_t i = N - 1;
            local_index[i]--;
            ptrdiff_t offset = -range->global_dim_strides[i];
            while (i && (local_index[i] < 0)) {
                offset += range->dimensions[i] * range->global_dim_strides[i];
                local_index[i--] = range->dimensions[i] - 1;
                offset -= range->global_dim_strides[i];
                local_index[i]--;
            }
            global_offset += offset;
            return *this;
        }

        multi_dimensional_iterator operator--(int) {
            auto cpy = *this;
            --(*this);
            return cpy;
        }

        inline multi_dimensional_iterator &operator++() {
            size_t i = N - 1;
            local_index[i]++;
            ptrdiff_t offset = range->global_dim_strides[i];
            while (i && (local_index[i] == range->dimensions[i])) {
                offset -= range->dimensions[i] * range->global_dim_strides[i];
                local_index[i--] = 0;
                offset += range->global_dim_strides[i];
                local_index[i]++;
            }
            global_offset += offset;
            return *this;
        }

        multi_dimensional_iterator operator++(int) {
            auto cpy = *this;
            ++(*this);
            return cpy;
        }

        pointer operator->() { return range->data[global_offset]; }

        pointer operator->() const { return range->data[global_offset]; }

        reference operator*() { return range->data[global_offset]; }

        const_reference operator*() const { return range->data[global_offset]; }

        bool operator==(multi_dimensional_iterator const &rhs) const { return global_offset == rhs.global_offset; }

        bool operator!=(multi_dimensional_iterator const &rhs) const { return global_offset != rhs.global_offset; }

        std::array<size_t, N> get_global_index() const {
            auto offset = global_offset;
            std::array<size_t, N> global_idx{0};
            for (int i = N - 1; i >= 0; i--) {
                global_idx[i] = offset % range->global_dimensions[i];
                offset /= range->global_dimensions[i];
            }
            return global_idx;
        }

        std::array<size_t, N> get_local_index() const { return local_index; }

        size_t get_local_index(size_t i) const { return local_index[i]; }

        ptrdiff_t get_offset() const { return global_offset; }

        std::array<size_t, N> get_dimensions() const { return range->get_dimensions(); }

        // assuming the iterator is at [i0, j0, k0, ...]
        // return the value of [i0 - pos[0], j0 - pos[1], k0 - pos[2] ...]
        // return 0 if range is exceeded
        // [input] offset for all the dimensions
        // [output] value of data at the target position
        template <class... Args>
        inline T prev(Args &&...pos) const {
            // TODO: check int type
            // TODO: change to offset map for efficiency
            static_assert(sizeof...(Args) == N, "Must have the same number of arguments");
            auto offset = global_offset;
            std::array<int, N> args{std::forward<Args>(pos)...};
            for (int i = 0; i < N; i++) {
                if (local_index[i] < args[i] && range->is_left_boundary(i)) return 0;
                offset -= args[i] ? args[i] * range->global_dim_strides[i] : 0;
            }
            return range->data[offset];
        }

        // No support for carry set.
        // For example, iterator in position (4,4) and dimension is 6x6, move(1,1) is supported but move (2,0) is not
        // supported.
        template <class... Args>
        multi_dimensional_iterator &move(Args &&...pos) {
            static_assert(sizeof...(Args) == N, "Must have the same number of arguments");
            std::array<int, N> args{std::forward<Args>(pos)...};
            return move2(args);
        }

        multi_dimensional_iterator &move2(std::array<int, N> args) {
            for (int i = N - 1; i >= 0; i--) {
                if (args[i]) {
                    assert(0 <= local_index[i] + args[i]);
                    assert(local_index[i] + args[i] < range->dimensions[i]);
                    local_index[i] += args[i];
                    global_offset += args[i] * range->global_dim_strides[i];
                }
            }
            return *this;
        }

        // No support for carry set.
        template <class... Args>
        multi_dimensional_iterator &move() {
            if (local_index[N - 1] < range->dimensions[N - 1] - 1) {
                local_index[N - 1]++;
                global_offset += range->global_dim_strides[N - 1];
            }
            return *this;
        }

        void print() {
            std::cout << "(";
            for (auto const &i : local_index) {
                std::cout << i << ",";
            }
            std::cout << "),[";
            for (auto const &i : range->dimensions) {
                std::cout << i << ",";
            }
            std::cout << "]" << std::endl;
        }

       private:
        friend multi_dimensional_range;
        std::shared_ptr<multi_dimensional_range> range;
        std::array<size_t, N> local_index;  // index of current_offset position
        ptrdiff_t global_offset;
    };

    using iterator = multi_dimensional_iterator;
    using const_iterator = multi_dimensional_iterator;
    using value_type = T;
    using reference = T &;
    using pointer = T *;

    template <class ForwardIt1>
    multi_dimensional_range(T *data_, ForwardIt1 global_dims_begin, ForwardIt1 global_dims_end, size_t stride_,
                            ptrdiff_t offset_)
        : left_boundary{false}, data(data_) {
        static_assert(std::is_convertible<typename std::iterator_traits<ForwardIt1>::value_type, std::size_t>::value,
                      "ForwardIt1 must be convertible to std::size_t");
        if (global_dims_end - global_dims_begin != N) {
            std::stringstream ss;
            ss << "#dimensions does not match: got " << global_dims_end - global_dims_begin << ", expected " << N;
            throw std::invalid_argument(ss.str());
        }
        set_access_stride(stride_);
        // set global dimensions
        int i = 0;
        for (auto iter = global_dims_begin; iter != global_dims_end; ++iter) {
            global_dimensions[i++] = *iter;
        }
        //            size_t cur_stride = stride_;
        //            for (int i = N - 1; i >= 0; i--) {
        //                global_dim_strides[i] = cur_stride;
        //                cur_stride *= global_dimensions[i];
        //            }
        // set_dimensions(dims_begin, dims_end);
        set_dimensions_auto();
        set_global_dim_strides();
        set_offsets(offset_);
    }

    multi_dimensional_range(T *data_, std::array<size_t, N> global_dims_, std::array<size_t, N> stride_,
                            ptrdiff_t offset_)
        : data(data_), left_boundary{false} {
        set_access_stride(stride_);
        // set global dimensions
        global_dimensions = global_dims_;
        set_dimensions_auto();
        set_global_dim_strides();
        set_offsets(offset_);
    }

    multi_dimensional_iterator begin() { return multi_dimensional_iterator(this->shared_from_this(), start_offset); }

    multi_dimensional_iterator end() { return multi_dimensional_iterator(this->shared_from_this(), end_offset); }

    template <class ForwardIt1>
    void set_dimensions(ForwardIt1 begin, ForwardIt1 end) {
        int i = 0;
        for (auto iter = begin; iter != end; ++iter) {
            dimensions[i++] = *iter;
            // std::cout << dimensions[i-1] << " ";
        }
    }

    void set_dimensions_auto() {
        // std::cout << "dimensions: ";
        for (size_t i = 0; i < dimensions.size(); i++) {
            // std::cout << "g[i]=" << global_dimensions[i] << ",str=" << access_stride << " ";
            dimensions[i] = (global_dimensions[i] - 1) / access_stride[i] + 1;
            // std::cout << dimensions[i] << " ";
        }
        // std::cout << std::endl;
    }

    void set_global_dim_strides() {
        // std::cout << "strides: ";
        size_t cur_stride = 1;
        for (int i = N - 1; i >= 0; i--) {
            global_dim_strides[i] = cur_stride * access_stride[i];
            cur_stride *= global_dimensions[i];
            // std::cout << dim_strides[i] << " ";
        }
        // std::cout << std::endl;
    }

    void set_offsets(ptrdiff_t offset_) {
        start_offset = offset_;
        end_offset = start_offset + dimensions[0] * global_dim_strides[0];
    }

    void set_access_stride(size_t stride_) { access_stride.fill(stride_); }

    void set_access_stride(std::array<size_t, N> stride_) { access_stride = stride_; }

    void update_block_range(multi_dimensional_iterator block, size_t block_size) {
        std::array<size_t, N> dims;
        for (int i = 0; i < N; i++) {
            // check boundary condition
            if (block.local_index[i] == block.range->dimensions[i] - 1) {
                dims[i] = global_dimensions[i] - block.local_index[i] * block.range->access_stride[i];
            } else {
                dims[i] = block_size;
            }
        }
        update_block_range(block, dims);
    }

    void update_block_range(multi_dimensional_iterator block, const std::array<size_t, N> &dims) {
        dimensions = dims;
        for (int i = 0; i < N; i++) {
            left_boundary[i] = (block.get_local_index(i) == 0);
        }
        this->set_offsets(block.get_offset());
    }

    size_t get_dimensions(size_t i) const { return dimensions[i]; }

    std::array<size_t, N> get_dimensions() const { return dimensions; }

    std::array<size_t, N> get_global_dimensions() const { return global_dimensions; }

    bool is_left_boundary(size_t i) const { return left_boundary[i]; }

    T *get_data() { return data; }

   private:
    std::array<size_t, N> global_dimensions;
    std::array<size_t, N> global_dim_strides;
    std::array<size_t, N> dimensions;  // the dimensions
    std::array<bool, N>
        left_boundary;  // if current block is the left boundary, iterators go outside the boundary will return 0.
    std::array<size_t, N> access_stride;  // stride for access pattern
    ptrdiff_t start_offset;               // offset for start point
    ptrdiff_t end_offset;                 // offset for end point
    T *data;                              // data pointer
};

}  // namespace SZ3
#endif
