#ifndef SZ3_INTERPOLATION_DECOMPOSITION_HPP
#define SZ3_INTERPOLATION_DECOMPOSITION_HPP

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <type_traits>

#include "Decomposition.hpp"
#include "SZ3/def.hpp"
#include "SZ3/quantizer/Quantizer.hpp"
#include "SZ3/utils/BlockwiseIterator.hpp"
#include "SZ3/utils/Config.hpp"
#include "SZ3/utils/FileUtil.hpp"
#include "SZ3/utils/Interpolators.hpp"
#include "SZ3/utils/Iterator.hpp"
#include "SZ3/utils/MemoryUtil.hpp"
#include "SZ3/utils/Timer.hpp"

namespace SZ3 {
template <class T, uint N, class Quantizer>
class InterpolationDecomposition : public concepts::DecompositionInterface<T, int, N> {
    static_assert(std::is_floating_point<T>::value, "InterpolationDecomposition only takes a floating-point T");

   public:
    InterpolationDecomposition(const Config & /*conf*/, Quantizer quantizer_) : quantizer(quantizer_) {
        static_assert(std::is_base_of<concepts::QuantizerInterface<T, int>, Quantizer>::value,
                      "must implement the quantizer interface");
    }

    T *decompress(const Config &conf, std::vector<int> &quant_inds_, T *dec_data) override {
        // load() read original_dimensions from the payload, and the grid walk below is sized by it while
        // dec_data and quant_inds are sized by conf. A tampered value runs off both.
        if (conf.dims.size() != N) {
            throw std::out_of_range("SZ3 interpolation: configuration dimension count does not match the data");
        }
        for (uint i = 0; i < N; i++) {
            if (original_dimensions[i] != conf.dims[i]) {
                throw std::out_of_range("SZ3 interpolation: stored dimensions do not match the trusted configuration");
            }
        }

        init();

        if (quant_inds_.size() < num_elements) {
            throw std::out_of_range("SZ3 interpolation: fewer bins than the grid consumes");
        }
        this->quant_inds = quant_inds_.data();
        double eb = quantizer.get_eb();

        if (anchor_stride == 0) {                                               // check whether used anchor points
            *dec_data = quantizer.recover(0, this->quant_inds[quant_index++]);  // no anchor points
        } else {
            recover_anchor_grid(dec_data);  // recover anchor points
            interp_level--;
        }

        for (int level = interp_level; level > 0 && level <= interp_level; level--) {
            // set level-wise error bound
            if (eb_alpha < 0) {
                if (level >= 3) {
                    quantizer.set_eb(eb * eb_ratio);
                } else {
                    quantizer.set_eb(eb);
                }
            } else if (eb_alpha >= 1) {
                double cur_ratio = pow(eb_alpha, level - 1);
                if (cur_ratio > eb_beta) {
                    cur_ratio = eb_beta;
                }
                quantizer.set_eb(eb / cur_ratio);
            }
            size_t stride = 1U << (level - 1);
            auto interp_block_size = blocksize * stride;
            auto inter_block_range = std::make_shared<multi_dimensional_range<T, N>>(
                dec_data, std::begin(original_dimensions), std::end(original_dimensions), interp_block_size, 0);
            auto inter_begin = inter_block_range->begin();
            auto inter_end = inter_block_range->end();
            for (auto block = inter_begin; block != inter_end; ++block) {
                auto end_idx = block.get_global_index();
                for (uint i = 0; i < N; i++) {
                    end_idx[i] += interp_block_size;
                    if (end_idx[i] > original_dimensions[i] - 1) {
                        end_idx[i] = original_dimensions[i] - 1;
                    }
                }
                interpolation(
                    dec_data, block.get_global_index(), end_idx, interpolators[interp_id],
                    [&](size_t /*idx*/, T &d, T pred) { d = quantizer.recover(pred, quant_inds_[quant_index++]); },
                    direction_sequence_id, stride);
            }
        }
        quantizer.postdecompress_data();
        return dec_data;
    }

    // compress given the error bound
    std::vector<int> compress(const Config &conf, T *data) override {
        std::copy_n(conf.dims.begin(), N, original_dimensions.begin());

        interp_id = conf.interpAlgo;
        direction_sequence_id = conf.interpDirection;
        anchor_stride = conf.interpAnchorStride;
        blocksize = 32;  // a empirical value. Can be very large but not helpful
        eb_alpha = conf.interpAlpha;
        eb_beta = conf.interpBeta;

        init();
        std::vector<int> quant_inds_vec(num_elements);
        quant_inds = quant_inds_vec.data();
        double eb = quantizer.get_eb();
        if (anchor_stride == 0) {  // check whether to use anchor points
            quant_inds[quant_index++] = quantizer.quantize_and_overwrite(*data, 0);  // no
        } else {
            build_anchor_grid(data);  // losslessly saving anchor points
            interp_level--;
        }

        for (int level = interp_level; level > 0 && level <= interp_level; level--) {
            double cur_eb = eb;
            // set level-wise error bound
            if (eb_alpha < 0) {
                if (level >= 3) {
                    cur_eb = eb * eb_ratio;
                } else {
                    cur_eb = eb;
                }
            } else if (eb_alpha >= 1) {
                double cur_ratio = pow(eb_alpha, level - 1);
                if (cur_ratio > eb_beta) {
                    cur_ratio = eb_beta;
                }
                cur_eb = eb / cur_ratio;
            }
            quantizer.set_eb(cur_eb);
            size_t stride = 1U << (level - 1);

            auto interp_block_size = blocksize * stride;

            auto inter_block_range = std::make_shared<multi_dimensional_range<T, N>>(
                data, std::begin(original_dimensions), std::end(original_dimensions), interp_block_size, 0);

            auto inter_begin = inter_block_range->begin();
            auto inter_end = inter_block_range->end();

            for (auto block = inter_begin; block != inter_end; ++block) {
                auto end_idx = block.get_global_index();
                for (uint i = 0; i < N; i++) {
                    end_idx[i] += interp_block_size;
                    if (end_idx[i] > original_dimensions[i] - 1) {
                        end_idx[i] = original_dimensions[i] - 1;
                    }
                }

                interpolation(
                    data, block.get_global_index(), end_idx, interpolators[interp_id],
                    [&](size_t /*idx*/, T &d, T pred) {
                        quant_inds[quant_index++] = (quantizer.quantize_and_overwrite(d, pred));
                    },
                    direction_sequence_id, stride);
            }
        }
        quantizer.set_eb(eb);
        quantizer.postcompress_data();
        return quant_inds_vec;
    }

    size_t size_est() override {
        return N * sizeof(uint64_t) + sizeof(blocksize) + sizeof(interp_id) + sizeof(direction_sequence_id) +
               quantizer.size_est() + 128;
    }

    void save(uchar *&c) override {
        for (size_t d : original_dimensions) write<uint64_t>(d, c);
        write(blocksize, c);
        write(interp_id, c);
        write(direction_sequence_id, c);
        write<uint64_t>(anchor_stride, c);
        write(eb_alpha, c);
        write(eb_beta, c);

        quantizer.save(c);
    }

    void load(const uchar *&c, size_t &remaining_length) override {
        uint64_t v = 0;
        for (size_t &d : original_dimensions) {
            read(v, c, remaining_length);
            d = v;
        }
        read(blocksize, c, remaining_length);
        read(interp_id, c, remaining_length);
        read(direction_sequence_id, c, remaining_length);
        read(v, c, remaining_length);
        anchor_stride = v;
        read(eb_alpha, c, remaining_length);
        read(eb_beta, c, remaining_length);

        quantizer.load(c, remaining_length);
    }

    std::pair<int, int> get_out_range() override { return quantizer.get_out_range(); }

   private:
    void init() {
        quant_index = 0;
        // All four come from the compressed payload; interp_id and direction_sequence_id index the fixed
        // tables built below.
        if (blocksize == 0 || blocksize % 2 != 0) {
            throw std::out_of_range("SZ3 interpolation: block size must be a positive even number");
        }
        if (interp_id < 0 || static_cast<size_t>(interp_id) >= interpolators.size()) {
            throw std::out_of_range("SZ3 interpolation: interpolator id is out of range");
        }
        num_elements = 1;
        interp_level = -1;
	bool use_anchor = false;
        for (uint i = 0; i < N; i++) {
            if (interp_level < ceil(log2(original_dimensions[i]))) {
                interp_level = static_cast<int>(ceil(log2(original_dimensions[i])));
            }
	    if (original_dimensions[i] > anchor_stride)
	        use_anchor = true;
            num_elements *= original_dimensions[i];
        }
        if (!use_anchor)
            anchor_stride = 0;
        // log2() and the anchor grid below need a power of two; a stride wider than the data is
        // already zero by here.
        if (anchor_stride > 0 && (anchor_stride & (anchor_stride - 1)) != 0) {
            throw std::out_of_range("SZ3 interpolation: anchor stride must be a power of two");
        }
        if (anchor_stride > 0) {
            int max_interpolation_level = static_cast<int>(log2(anchor_stride)) + 1;
            if (max_interpolation_level <= interp_level) {
                interp_level = max_interpolation_level;
            }
        }

        original_dim_offsets[N - 1] = 1;
        for (int i = N - 2; i >= 0; i--) {
            original_dim_offsets[i] = original_dim_offsets[i + 1] * original_dimensions[i + 1];
        }

        dim_sequences = std::vector<std::array<int, N>>();
        auto sequence = std::array<int, N>();
        for (uint i = 0; i < N; i++) {
            sequence[i] = i;
        }
        do {
            dim_sequences.push_back(sequence);
        } while (std::next_permutation(sequence.begin(), sequence.end()));
        if (direction_sequence_id < 0 || static_cast<size_t>(direction_sequence_id) >= dim_sequences.size()) {
            throw std::out_of_range("SZ3 interpolation: direction sequence id is out of range");
        }
    }

    void build_anchor_grid(T *data) {  // store anchor points. steplength: anchor_stride on each dimension
        std::array<size_t, N> strides;
        std::array<size_t, N> begins{0};
        std::fill(strides.begin(), strides.end(), anchor_stride);
        foreach
            <T, N>(data, 0, begins, original_dimensions, strides, original_dim_offsets,
                   [&](T *d) { quant_inds[quant_index++] = quantizer.force_save_unpred(*d); });
    }

    void recover_anchor_grid(T *data) {  // recover anchor points. steplength: anchor_stride on each dimension
        std::array<size_t, N> strides;
        std::array<size_t, N> begins{0};
        std::fill(strides.begin(), strides.end(), anchor_stride);
        foreach
            <T, N>(data, 0, begins, original_dimensions, strides, original_dim_offsets, [&](T *d) {
                *d = quantizer.recover_unpred();
                quant_index++;
            });
    }

    /**
     * Do interpolations along a certain dimension, and move through that dimension only.
     * This is the original API, described in the ICDE'21 paper.
     */
    template <class QuantizeFunc>
    double interpolation_1d(T *data, size_t begin, size_t end, size_t stride, const std::string &interp_func,
                            QuantizeFunc &&quantize_func) {
        size_t n = (end - begin) / stride + 1;
        if (n <= 1) {
            return 0;
        }
        double predict_error = 0;

        size_t stride3x = 3 * stride;
        size_t stride5x = 5 * stride;
        if (interp_func == "linear" || n < 5) {
            // if (pb == PB_predict_overwrite) {
            for (size_t i = 1; i + 1 < n; i += 2) {
                T *d = data + begin + i * stride;
                quantize_func(d - data, *d, interp_linear(*(d - stride), *(d + stride)));
            }
            if (n % 2 == 0) {
                T *d = data + begin + (n - 1) * stride;
                if (n < 4) {
                    quantize_func(d - data, *d, *(d - stride));
                } else {
                    quantize_func(d - data, *d, interp_linear1(*(d - stride3x), *(d - stride)));
                }
            }
            // }
        } else {
            T *d;
            size_t i;
            for (i = 3; i + 3 < n; i += 2) {
                d = data + begin + i * stride;
                quantize_func(d - data, *d,
                              interp_cubic(*(d - stride3x), *(d - stride), *(d + stride), *(d + stride3x)));
            }
            d = data + begin + stride;
            quantize_func(d - data, *d, interp_quad_1(*(d - stride), *(d + stride), *(d + stride3x)));

            d = data + begin + i * stride;
            quantize_func(d - data, *d, interp_quad_2(*(d - stride3x), *(d - stride), *(d + stride)));
            if (n % 2 == 0) {
                d = data + begin + (n - 1) * stride;
                quantize_func(d - data, *d, interp_quad_3(*(d - stride5x), *(d - stride3x), *(d - stride)));
            }
        }

        return predict_error;
    }

    /**
     * Do all interpolations along a certain dimension on the full data grid. Moving on the fastest-dim.
     * This is the new API, described in the SIGMOD'24 paper.
     */
    template <class QuantizeFunc>
    double interpolation_1d_fastest_dim_first(T *data, const std::array<size_t, N> &begin_idx,
                                              const std::array<size_t, N> &end_idx, const size_t &direction,
                                              std::array<size_t, N> &strides, const size_t &math_stride,
                                              const std::string &interp_func, QuantizeFunc &&quantize_func) {
        for (size_t i = 0; i < N; i++) {
            if (end_idx[i] < begin_idx[i]) return 0;
        }
        size_t math_begin_idx = begin_idx[direction], math_end_idx = end_idx[direction];
        size_t n = (math_end_idx - math_begin_idx) / math_stride + 1;
        if (n <= 1) {
            return 0;
        }
        double predict_error = 0.0;
        size_t offset = 0;
        size_t stride = math_stride * original_dim_offsets[direction];
        std::array<size_t, N> begins, ends, dim_offsets;
        for (size_t i = 0; i < N; i++) {
            begins[i] = 0;
            ends[i] = end_idx[i] - begin_idx[i] + 1;
            dim_offsets[i] = original_dim_offsets[i];
            offset += original_dim_offsets[i] * begin_idx[i];
        }
        dim_offsets[direction] = stride;
        size_t stride2x = 2 * stride;
        if (interp_func == "linear") {
            begins[direction] = 1;
            ends[direction] = n - 1;
            strides[direction] = 2;
            foreach
                <T, N>(data, offset, begins, ends, strides, dim_offsets,
                       [&](T *d) { quantize_func(d - data, *d, interp_linear(*(d - stride), *(d + stride))); });
            if (n % 2 == 0) {
                begins[direction] = n - 1;
                ends[direction] = n;
                foreach
                    <T, N>(data, offset, begins, ends, strides, dim_offsets, [&](T *d) {
                        if (n < 3)
                            quantize_func(d - data, *d, *(d - stride));
                        else
                            quantize_func(d - data, *d, interp_linear1(*(d - stride2x), *(d - stride)));
                    });
            }
        } else {
            size_t stride3x = 3 * stride;
            size_t i_start = 3;
            begins[direction] = i_start;
            ends[direction] = (n >= 3) ? (n - 3) : 0;
            strides[direction] = 2;
            foreach
                <T, N>(data, offset, begins, ends, strides, dim_offsets, [&](T *d) {
                    quantize_func(d - data, *d,
                                  interp_cubic(*(d - stride3x), *(d - stride), *(d + stride), *(d + stride3x)));
                });
            std::vector<size_t> boundaries;
            boundaries.push_back(1);
            if (n % 2 == 1 && n > 3) {
                boundaries.push_back(n - 2);
            }
            if (n % 2 == 0 && n > 4) {
                boundaries.push_back(n - 3);
            }
            if (n % 2 == 0 && n > 2) {
                boundaries.push_back(n - 1);
            }
            for (auto boundary : boundaries) {
                begins[direction] = boundary;
                ends[direction] = boundary + 1;
                foreach
                    <T, N>(data, offset, begins, ends, strides, dim_offsets, [&](T *d) {
                        if (boundary >= 3) {
                            if (boundary + 3 < n)
                                quantize_func(
                                    d - data, *d,
                                    interp_cubic(*(d - stride3x), *(d - stride), *(d + stride), *(d + stride3x)));
                            else if (boundary + 1 < n)
                                quantize_func(d - data, *d,
                                              interp_quad_2(*(d - stride3x), *(d - stride), *(d + stride)));
                            else
                                quantize_func(d - data, *d, interp_linear1(*(d - stride3x), *(d - stride)));
                        } else {
                            if (boundary + 3 < n)
                                quantize_func(d - data, *d,
                                              interp_quad_1(*(d - stride), *(d + stride), *(d + stride3x)));
                            else if (boundary + 1 < n)
                                quantize_func(d - data, *d, interp_linear(*(d - stride), *(d + stride)));
                            else
                                quantize_func(d - data, *d, *(d - stride));
                        }
                    });
            }
        }
        return predict_error;
    }

    template <class QuantizeFunc>
    double interpolation(T *data, std::array<size_t, N> begin, std::array<size_t, N> end,
                         const std::string &interp_func, QuantizeFunc &&quantize_func, const int direction,
                         size_t stride = 1) {
        if constexpr (N == 1) {  // old API
            return interpolation_1d(data, begin[0], end[0], stride, interp_func, quantize_func);
        } else if constexpr (N == 2) {  // old API
            double predict_error = 0;
            size_t stride2x = stride * 2;
            const std::array<int, N> dims = dim_sequences[direction];
            for (size_t j = (begin[dims[1]] ? begin[dims[1]] + stride2x : 0); j <= end[dims[1]]; j += stride2x) {
                size_t begin_offset =
                    begin[dims[0]] * original_dim_offsets[dims[0]] + j * original_dim_offsets[dims[1]];
                predict_error += interpolation_1d(
                    data, begin_offset, begin_offset + (end[dims[0]] - begin[dims[0]]) * original_dim_offsets[dims[0]],
                    stride * original_dim_offsets[dims[0]], interp_func, quantize_func);
            }
            for (size_t i = (begin[dims[0]] ? begin[dims[0]] + stride : 0); i <= end[dims[0]]; i += stride) {
                size_t begin_offset =
                    i * original_dim_offsets[dims[0]] + begin[dims[1]] * original_dim_offsets[dims[1]];
                predict_error += interpolation_1d(
                    data, begin_offset, begin_offset + (end[dims[1]] - begin[dims[1]]) * original_dim_offsets[dims[1]],
                    stride * original_dim_offsets[dims[1]], interp_func, quantize_func);
            }
            return predict_error;
        } else if constexpr (N == 3 || N == 4) {  // new API (for faster speed)
            double predict_error = 0;
            size_t stride2x = stride * 2;
            const std::array<int, N> dims = dim_sequences[direction];
            std::array<size_t, N> strides;
            std::array<size_t, N> begin_idx = begin, end_idx = end;
            strides[dims[0]] = 1;
            for (uint i = 1; i < N; i++) {
                begin_idx[dims[i]] = (begin[dims[i]] ? begin[dims[i]] + stride2x : 0);
                strides[dims[i]] = stride2x;
            }

            predict_error += interpolation_1d_fastest_dim_first(data, begin_idx, end_idx, dims[0], strides, stride,
                                                                interp_func, quantize_func);
            for (uint i = 1; i < N; i++) {
                begin_idx[dims[i]] = begin[dims[i]];
                begin_idx[dims[i - 1]] = (begin[dims[i - 1]] ? begin[dims[i - 1]] + stride : 0);
                strides[dims[i - 1]] = stride;
                predict_error += interpolation_1d_fastest_dim_first(data, begin_idx, end_idx, dims[i], strides, stride,
                                                                    interp_func, quantize_func);
            }
            return predict_error;
        } else {
            throw std::runtime_error("Unsupported dimension in InterpolationDecomposition");
        }
    }

    int interp_level = -1;
    int interp_id;
    uint blocksize;
    std::vector<std::string> interpolators = {"linear", "cubic"};
    int *quant_inds;
    size_t quant_index = 0;
    double max_error;
    Quantizer quantizer;
    size_t num_elements;
    std::array<size_t, N> original_dimensions;
    std::array<size_t, N> original_dim_offsets;
    std::vector<std::array<int, N>> dim_sequences;
    int direction_sequence_id;
    size_t anchor_stride = 0;
    double eb_alpha = -1;
    double eb_beta = -1;
    double eb_ratio = 0.5;  // To be deprecated
};

template <class T, uint N, class Quantizer>
InterpolationDecomposition<T, N, Quantizer> make_decomposition_interpolation(const Config &conf, Quantizer quantizer) {
    return InterpolationDecomposition<T, N, Quantizer>(conf, quantizer);
}

}  // namespace SZ3

#endif
