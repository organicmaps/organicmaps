#pragma once

#include "bit_vector.hpp"
#include "broadword.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct {

    struct darray64
    {
        struct builder {
            builder()
                : n_ones(0)
            {}

            void append1(uint64_t skip0 = 0)
            {
                bits.append_bits(0, skip0);
                bits.push_back(1);

                if (n_ones % block_size == 0) {
                    block_inventory.push_back(bits.size() - 1);
                }
                if (n_ones % subblock_size == 0) {
                    subblock_inventory.push_back(uint16_t(bits.size() - 1 - block_inventory[n_ones / block_size]));
                }

                n_ones += 1;
            }

            uint64_t n_ones;
            bit_vector_builder bits;
            std::vector<uint64_t> block_inventory;
            std::vector<uint16_t> subblock_inventory;
        };

        darray64()
            : m_num_ones(0)
        {}

        darray64(builder* b)
        {
            m_num_ones = b->n_ones;
            bit_vector(&b->bits).swap(m_bits);
            m_block_inventory.steal(b->block_inventory);
            m_subblock_inventory.steal(b->subblock_inventory);
        }

        void swap(darray64& other)
        {
            std::swap(m_num_ones, other.m_num_ones);
            m_bits.swap(other.m_bits);
            m_block_inventory.swap(other.m_block_inventory);
            m_subblock_inventory.swap(other.m_subblock_inventory);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_num_ones, "m_num_ones")
                (m_bits, "m_bits")
                (m_block_inventory, "m_block_inventory")
                (m_subblock_inventory, "m_subblock_inventory")
                ;
        }

        uint64_t num_ones() const
        {
            return m_num_ones;
        }

        bit_vector const& bits() const
        {
            return m_bits;
        }

        uint64_t select(uint64_t idx) const
        {
            assert(idx < num_ones());
            uint64_t block = idx / block_size;
            uint64_t block_pos = m_block_inventory[block];

            uint64_t subblock = idx / subblock_size;
            uint64_t start_pos = block_pos + m_subblock_inventory[subblock];
            uint64_t reminder = idx % subblock_size;

            if (!reminder) {
                return start_pos;
            } else {
                uint64_t word_idx = start_pos / 64;
                uint64_t word_shift = start_pos % 64;
                uint64_t word = m_bits.data()[word_idx] & (uint64_t(-1) << word_shift);

                while (true) {
                    uint64_t popcnt = broadword::popcount(word);
                    if (reminder < popcnt) break;
                    reminder -= popcnt;
                    word = m_bits.data()[++word_idx];
                }

                return 64 * word_idx + broadword::select_in_word(word, reminder);
            }
        }

    protected:

        static const size_t block_size = 1024; // 64 * block_size must fit in an uint16_t (64 is the max sparsity of bits)
        static const size_t subblock_size = 64;

        uint64_t m_num_ones;
        bit_vector m_bits;
        mapper::mappable_vector<uint64_t> m_block_inventory;
        mapper::mappable_vector<uint16_t> m_subblock_inventory;

    };
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
