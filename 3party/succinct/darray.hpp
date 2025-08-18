#pragma once

#include "bit_vector.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct {

    namespace detail {

        template <typename WordGetter>
        class darray {
        public:

            darray()
                : m_positions()
            {}

            darray(bit_vector const& bv)
                : m_positions()
            {
                mapper::mappable_vector<uint64_t> const& data = bv.data();

                std::vector<uint64_t> cur_block_positions;
                std::vector<int64_t> block_inventory;
                std::vector<uint16_t> subblock_inventory;
                std::vector<uint64_t> overflow_positions;

                for (uint64_t word_idx = 0; word_idx < data.size(); ++word_idx) {
                    uint64_t cur_pos = word_idx * 64;
                    uint64_t cur_word = WordGetter()(data, word_idx);
                    unsigned long l;
                    while (broadword::lsb(cur_word, l)) {
                        cur_pos += l;
                        cur_word >>= l;
                        if (cur_pos >= bv.size()) break;

                        cur_block_positions.push_back(cur_pos);

                        if (cur_block_positions.size() == block_size) {
                            flush_cur_block(cur_block_positions, block_inventory, subblock_inventory, overflow_positions);
                        }

                        // can't do >>= l + 1, can be 64
                        cur_word >>= 1;
                        cur_pos += 1;
                        m_positions += 1;
                    }
                }
                if (cur_block_positions.size()) {
                    flush_cur_block(cur_block_positions, block_inventory, subblock_inventory, overflow_positions);
                }

                m_block_inventory.steal(block_inventory);
                m_subblock_inventory.steal(subblock_inventory);
                m_overflow_positions.steal(overflow_positions);
            }

            template <typename Visitor>
            void map(Visitor& visit) {
                visit
                    (m_positions, "m_positions")
                    (m_block_inventory, "m_block_inventory")
                    (m_subblock_inventory, "m_subblock_inventory")
                    (m_overflow_positions, "m_overflow_positions")
                    ;
            }

            void swap(darray& other) {
                std::swap(other.m_positions, m_positions);
                m_block_inventory.swap(other.m_block_inventory);
                m_subblock_inventory.swap(other.m_subblock_inventory);
                m_overflow_positions.swap(other.m_overflow_positions);
            }

            inline uint64_t select(bit_vector const& bv, uint64_t idx) const
            {
                assert(idx < num_positions());
                uint64_t block = idx / block_size;
                int64_t block_pos = m_block_inventory[block];
                if (block_pos < 0) {
                    uint64_t overflow_pos = uint64_t(-block_pos - 1);
                    return m_overflow_positions[overflow_pos + (idx % block_size)];
                }

                uint64_t subblock = idx / subblock_size;
                uint64_t start_pos = uint64_t(block_pos) + m_subblock_inventory[subblock];
                uint64_t reminder = idx % subblock_size;
                mapper::mappable_vector<uint64_t> const& data = bv.data();

                if (!reminder) {
                    return start_pos;
                } else {
                    uint64_t word_idx = start_pos / 64;
                    uint64_t word_shift = start_pos % 64;
                    uint64_t word = WordGetter()(data, word_idx) & (uint64_t(-1) << word_shift);

                    while (true) {
                        uint64_t popcnt = broadword::popcount(word);
                        if (reminder < popcnt) break;
                        reminder -= popcnt;
                        word = WordGetter()(data, ++word_idx);
                    }

                    return 64 * word_idx + broadword::select_in_word(word, reminder);
                }
            }

            inline uint64_t num_positions() const {
                return m_positions;
            }

        protected:

            static void flush_cur_block(std::vector<uint64_t>& cur_block_positions, std::vector<int64_t>& block_inventory,
                                        std::vector<uint16_t>& subblock_inventory, std::vector<uint64_t>& overflow_positions)
            {
                if (cur_block_positions.back() - cur_block_positions.front() < max_in_block_distance) {
                    block_inventory.push_back(int64_t(cur_block_positions.front()));
                    for (size_t i = 0; i < cur_block_positions.size(); i += subblock_size) {
                        subblock_inventory.push_back(uint16_t(cur_block_positions[i] - cur_block_positions.front()));
                    }
                } else {
                    block_inventory.push_back(-int64_t(overflow_positions.size()) - 1);
                    for (size_t i = 0; i < cur_block_positions.size(); ++i) {
                        overflow_positions.push_back(cur_block_positions[i]);
                    }
                    for (size_t i = 0; i < cur_block_positions.size(); i += subblock_size) {
                        subblock_inventory.push_back(uint16_t(-1));
                    }
                }
                cur_block_positions.clear();
            }



            static const size_t block_size = 1024;
            static const size_t subblock_size = 32;
            static const size_t max_in_block_distance = 1 << 16;

            uint64_t m_positions;
            mapper::mappable_vector<int64_t> m_block_inventory;
            mapper::mappable_vector<uint16_t> m_subblock_inventory;
            mapper::mappable_vector<uint64_t> m_overflow_positions;
        };

        struct identity_getter
        {
            uint64_t operator()(mapper::mappable_vector<uint64_t> const& data, size_t idx) const {
                return data[idx];
            }
        };

        struct negating_getter
        {
            uint64_t operator()(mapper::mappable_vector<uint64_t> const& data, size_t idx) const {
                return ~data[idx];
            }
        };
    }

    typedef detail::darray<detail::identity_getter> darray1;
    typedef detail::darray<detail::negating_getter> darray0;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
