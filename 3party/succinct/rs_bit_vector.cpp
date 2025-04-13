#include "rs_bit_vector.hpp"

namespace succinct {

    void rs_bit_vector::build_indices(bool with_select_hints, bool with_select0_hints)
    {
        {
            using broadword::popcount;
            std::vector<uint64_t> block_rank_pairs;
            uint64_t next_rank = 0;
            uint64_t cur_subrank = 0;
            uint64_t subranks = 0;
            block_rank_pairs.push_back(0);
            for (uint64_t i = 0; i < m_bits.size(); ++i) {
                uint64_t word_pop = popcount(m_bits[i]);
                uint64_t shift = i % block_size;
                if (shift) {
                    subranks <<= 9;
                    subranks |= cur_subrank;
                }
                next_rank += word_pop;
                cur_subrank += word_pop;

                if (shift == block_size - 1) {
                    block_rank_pairs.push_back(subranks);
                    block_rank_pairs.push_back(next_rank);
                    subranks = 0;
                    cur_subrank = 0;
                }
            }
            uint64_t left = block_size - m_bits.size() % block_size;
            for (uint64_t i = 0; i < left; ++i) {
                subranks <<= 9;
                subranks |= cur_subrank;
            }
            block_rank_pairs.push_back(subranks);

            if (m_bits.size() % block_size) {
                block_rank_pairs.push_back(next_rank);
                block_rank_pairs.push_back(0);
            }

            m_block_rank_pairs.steal(block_rank_pairs);
        }

        if (with_select_hints) {
            std::vector<uint64_t> select_hints;
            uint64_t cur_ones_threshold = select_ones_per_hint;
            for (uint64_t i = 0; i < num_blocks(); ++i) {
                if (block_rank(i + 1) > cur_ones_threshold) {
                    select_hints.push_back(i);
                    cur_ones_threshold += select_ones_per_hint;
                }
            }
            select_hints.push_back(num_blocks());
            m_select_hints.steal(select_hints);
        }

        if (with_select0_hints) {
            std::vector<uint64_t> select0_hints;
            uint64_t cur_zeros_threshold = select_zeros_per_hint;
            for (uint64_t i = 0; i < num_blocks(); ++i) {
                if (block_rank0(i + 1) > cur_zeros_threshold) {
                    select0_hints.push_back(i);
                    cur_zeros_threshold += select_zeros_per_hint;
                }
            }
            select0_hints.push_back(num_blocks());
            m_select0_hints.steal(select0_hints);
        }
    }

}
