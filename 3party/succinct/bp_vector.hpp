#pragma once

#include <vector>
#include <algorithm>
#include <limits>

#include "rs_bit_vector.hpp"

namespace succinct {

    class bp_vector : public rs_bit_vector {
    public:
        bp_vector()
            : rs_bit_vector()
        {}

        template <class Range>
        bp_vector(Range const& from,
                  bool with_select_hints = false,
                  bool with_select0_hints = false)
            : rs_bit_vector(from, with_select_hints, with_select0_hints)
        {
            build_min_tree();
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            rs_bit_vector::map(visit);
            visit
                (m_internal_nodes, "m_internal_nodes")
                (m_block_excess_min, "m_block_excess_min")
                (m_superblock_excess_min, "m_superblock_excess_min")
                ;
        }

        void swap(bp_vector& other) {
            rs_bit_vector::swap(other);
            std::swap(m_internal_nodes, other.m_internal_nodes);
            m_block_excess_min.swap(other.m_block_excess_min);
            m_superblock_excess_min.swap(other.m_superblock_excess_min);
        }

        uint64_t find_open(uint64_t pos) const;
        uint64_t find_close(uint64_t pos) const;
        uint64_t enclose(uint64_t pos) const {
            assert((*this)[pos]);
            return find_open(pos);
        }

        typedef int32_t excess_t; // Allow at most 2^31 depth of the tree

        excess_t excess(uint64_t pos) const;
        uint64_t excess_rmq(uint64_t a, uint64_t b, excess_t& min_exc) const;
        inline uint64_t excess_rmq(uint64_t a, uint64_t b) const {
            excess_t foo;
            return excess_rmq(a, b, foo);
        }


    protected:

        static const size_t bp_block_size = 4; // to increase confusion, bp block_size is not necessarily rs_bit_vector block_size
        static const size_t superblock_size = 32; // number of blocks in superblock

        typedef int16_t block_min_excess_t; // superblock must be at most 2^15 - 1 bits

        bool find_close_in_block(uint64_t pos, excess_t excess,
                                 uint64_t max_sub_blocks, uint64_t& ret) const;
        bool find_open_in_block(uint64_t pos, excess_t excess,
                                uint64_t max_sub_blocks, uint64_t& ret) const;

        void excess_rmq_in_block(uint64_t start, uint64_t end,
                                 bp_vector::excess_t& exc,
                                 bp_vector::excess_t& min_exc,
                                 uint64_t& min_exc_idx) const;
        void excess_rmq_in_superblock(uint64_t block_start, uint64_t block_end,
                                      bp_vector::excess_t& block_min_exc,
                                      uint64_t& block_min_idx) const;
        void find_min_superblock(uint64_t superblock_start, uint64_t superblock_end,
                                 bp_vector::excess_t& superblock_min_exc,
                                 uint64_t& superblock_min_idx) const;


        inline excess_t get_block_excess(uint64_t block) const;
        inline bool in_node_range(uint64_t node, excess_t excess) const;

        template <int direction>
        inline bool search_block_in_superblock(uint64_t block, excess_t excess, size_t& found_block) const;

        template <int direction>
        inline uint64_t search_min_tree(uint64_t block, excess_t excess) const;

        void build_min_tree();

        uint64_t m_internal_nodes;
        mapper::mappable_vector<block_min_excess_t> m_block_excess_min;
        mapper::mappable_vector<excess_t> m_superblock_excess_min;
    };
}
