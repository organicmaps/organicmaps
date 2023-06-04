#pragma once

#include <vector>
#include <algorithm>

#include "bit_vector.hpp"
#include "broadword.hpp"

namespace succinct {

    class rs_bit_vector : public bit_vector {
    public:
        rs_bit_vector()
            : bit_vector()
        {}

        template <class Range>
        rs_bit_vector(Range const& from,
                      bool with_select_hints = false,
                      bool with_select0_hints = false)
            : bit_vector(from)
        {
            build_indices(with_select_hints, with_select0_hints);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            bit_vector::map(visit);
            visit
                (m_block_rank_pairs, "m_block_rank_pairs")
                (m_select_hints, "m_select_hints")
                (m_select0_hints, "m_select0_hints")
                ;
        }

        void swap(rs_bit_vector& other) {
            bit_vector::swap(other);
            m_block_rank_pairs.swap(other.m_block_rank_pairs);
            m_select_hints.swap(other.m_select_hints);
            m_select0_hints.swap(other.m_select0_hints);
        }

        inline uint64_t num_ones() const {
            return *(m_block_rank_pairs.end() - 2);
        }

        inline uint64_t num_zeros() const {
            return size() - num_ones();
        }

        inline uint64_t rank(uint64_t pos) const {
            assert(pos <= size());
            if (pos == size()) {
                return num_ones();
            }

            uint64_t sub_block = pos / 64;
            uint64_t r = sub_block_rank(sub_block);
            uint64_t sub_left = pos % 64;
            if (sub_left) {
                r += broadword::popcount(m_bits[sub_block] << (64 - sub_left));
            }
            return r;
        }

        inline uint64_t rank0(uint64_t pos) const {
            return pos - rank(pos);
        }

        inline uint64_t select(uint64_t n) const {
            using broadword::popcount;
            using broadword::select_in_word;
            assert(n < num_ones());
            uint64_t a = 0;
            uint64_t b = num_blocks();
            if (m_select_hints.size()) {
                uint64_t chunk = n / select_ones_per_hint;
                if (chunk != 0) {
                    a = m_select_hints[chunk - 1];
                }
                b = m_select_hints[chunk] + 1;
            }

            uint64_t block = 0;
            while (b - a > 1) {
                uint64_t mid = a + (b - a) / 2;
                uint64_t x = block_rank(mid);
                if (x <= n) {
                    a = mid;
                } else {
                    b = mid;
                }
            }
            block = a;

            assert(block < num_blocks());
            uint64_t block_offset = block * block_size;
            uint64_t cur_rank = block_rank(block);
            assert(cur_rank <= n);


            uint64_t rank_in_block_parallel = (n - cur_rank) * broadword::ones_step_9;
            uint64_t sub_ranks = sub_block_ranks(block);
            uint64_t sub_block_offset = broadword::uleq_step_9(sub_ranks, rank_in_block_parallel) * broadword::ones_step_9 >> 54 & 0x7;
            cur_rank += sub_ranks >> (7 - sub_block_offset) * 9 & 0x1FF;
            assert(cur_rank <= n);

            uint64_t word_offset = block_offset + sub_block_offset;
            return word_offset * 64 + select_in_word(m_bits[word_offset], n - cur_rank);
        }

        // TODO(ot): share code between select and select0
        inline uint64_t select0(uint64_t n) const {
            using broadword::popcount;
            using broadword::select_in_word;
            assert(n < num_zeros());
            uint64_t a = 0;
            uint64_t b = num_blocks();
            if (m_select0_hints.size()) {
                uint64_t chunk = n / select_zeros_per_hint;
                if (chunk != 0) {
                    a = m_select0_hints[chunk - 1];
                }
                b = m_select0_hints[chunk] + 1;
            }

            uint64_t block = 0;
            while (b - a > 1) {
                uint64_t mid = a + (b - a) / 2;
                uint64_t x = block_rank0(mid);
                if (x <= n) {
                    a = mid;
                } else {
                    b = mid;
                }
            }
            block = a;

            assert(block < num_blocks());
            uint64_t block_offset = block * block_size;
            uint64_t cur_rank0 = block_rank0(block);
            assert(cur_rank0 <= n);

            uint64_t rank_in_block_parallel = (n - cur_rank0) * broadword::ones_step_9;
            uint64_t sub_ranks = 64 * broadword::inv_count_step_9 - sub_block_ranks(block);
            uint64_t sub_block_offset = broadword::uleq_step_9(sub_ranks, rank_in_block_parallel) * broadword::ones_step_9 >> 54 & 0x7;
            cur_rank0 += sub_ranks >> (7 - sub_block_offset) * 9 & 0x1FF;
            assert(cur_rank0 <= n);

            uint64_t word_offset = block_offset + sub_block_offset;
            return word_offset * 64 + select_in_word(~m_bits[word_offset], n - cur_rank0);
        }

    protected:

        inline uint64_t num_blocks() const {
            return m_block_rank_pairs.size() / 2 - 1;
        }

        inline uint64_t block_rank(uint64_t block) const {
            return m_block_rank_pairs[block * 2];
        }

        inline uint64_t sub_block_rank(uint64_t sub_block) const {
            uint64_t r = 0;
            uint64_t block = sub_block / block_size;
            r += block_rank(block);
            uint64_t left = sub_block % block_size;
            r += sub_block_ranks(block) >> ((7 - left) * 9) & 0x1FF;
            return r;
        }

        inline uint64_t sub_block_ranks(uint64_t block) const {
            return m_block_rank_pairs[block * 2 + 1];
        }

        inline uint64_t block_rank0(uint64_t block) const {
            return block * block_size * 64 - m_block_rank_pairs[block * 2];
        }

        void build_indices(bool with_select_hints, bool with_select0_hints);

        static const uint64_t block_size = 8; // in 64bit words
        static const uint64_t select_ones_per_hint = 64 * block_size * 2; // must be > block_size * 64
        static const uint64_t select_zeros_per_hint = select_ones_per_hint;

        typedef mapper::mappable_vector<uint64_t> uint64_vec;
        uint64_vec m_block_rank_pairs;
        uint64_vec m_select_hints;
        uint64_vec m_select0_hints;
    };
}
