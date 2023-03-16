/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef RANGE_TABLE_HPP
#define RANGE_TABLE_HPP

#include "../util/integer_range.hpp"
#include "shared_memory_factory.hpp"
#include "shared_memory_vector_wrapper.hpp"

#include <fstream>
#include <vector>
#include <array>

/*
 * These pre-declarations are needed because parsing C++ is hard
 * and otherwise the compiler gets confused.
 */

template <unsigned BLOCK_SIZE = 16, bool USE_SHARED_MEMORY = false> class RangeTable;

template <unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::ostream &operator<<(std::ostream &out, const RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table);

template <unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::istream &operator>>(std::istream &in, RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table);

/**
 * Stores adjacent ranges in a compressed format.
 *
 * Maximum supported length of a range is 255.
 *
 * Note: BLOCK_SIZE is the number of differential encodoed values.
 * But each block consists of an absolute value and BLOCK_SIZE differential values.
 * So the effective block size is sizeof(unsigned) + BLOCK_SIZE.
 */
template <unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY> class RangeTable
{
  public:
    using BlockT = std::array<unsigned char, BLOCK_SIZE>;
    using BlockContainerT = typename ShM<BlockT, USE_SHARED_MEMORY>::vector;
    using OffsetContainerT = typename ShM<unsigned, USE_SHARED_MEMORY>::vector;
    using RangeT = osrm::range<unsigned>;

    friend std::ostream &operator<<<>(std::ostream &out, const RangeTable &table);
    friend std::istream &operator>><>(std::istream &in, RangeTable &table);

    RangeTable() : sum_lengths(0) {}

    // for loading from shared memory
    explicit RangeTable(OffsetContainerT &external_offsets,
                        BlockContainerT &external_blocks,
                        const unsigned sum_lengths)
        : sum_lengths(sum_lengths)
    {
        block_offsets.swap(external_offsets);
        diff_blocks.swap(external_blocks);
    }

    // construct table from length vector
    explicit RangeTable(const std::vector<unsigned> &lengths)
    {
        const unsigned number_of_blocks = [&lengths]()
        {
            unsigned num = (lengths.size() + 1) / (BLOCK_SIZE + 1);
            if ((lengths.size() + 1) % (BLOCK_SIZE + 1) != 0)
            {
                num += 1;
            }
            return num;
        }();

        block_offsets.reserve(number_of_blocks);
        diff_blocks.reserve(number_of_blocks);

        unsigned last_length = 0;
        unsigned lengths_prefix_sum = 0;
        unsigned block_idx = 0;
        unsigned block_counter = 0;
        BlockT block;
        unsigned block_sum = 0;
        for (const unsigned l : lengths)
        {
            // first entry of a block: encode absolute offset
            if (block_idx == 0)
            {
                block_offsets.push_back(lengths_prefix_sum);
                block_sum = 0;
            }
            else
            {
                block[block_idx - 1] = last_length;
                block_sum += last_length;
            }

            BOOST_ASSERT((block_idx == 0 && block_offsets[block_counter] == lengths_prefix_sum) ||
                         lengths_prefix_sum == (block_offsets[block_counter] + block_sum));

            // block is full
            if (BLOCK_SIZE == block_idx)
            {
                diff_blocks.push_back(block);
                block_counter++;
            }

            // we can only store strings with length 255
            BOOST_ASSERT(l <= 255);

            lengths_prefix_sum += l;
            last_length = l;

            block_idx = (block_idx + 1) % (BLOCK_SIZE + 1);
        }

        // Last block can't be finished because we didn't add the sentinel
        BOOST_ASSERT(block_counter == (number_of_blocks - 1));

        // one block missing: starts with guard value
        if (0 == block_idx)
        {
            // the last value is used as sentinel
            block_offsets.push_back(lengths_prefix_sum);
            block_idx = 1;
            last_length = 0;
        }

        while (0 != block_idx)
        {
            block[block_idx - 1] = last_length;
            last_length = 0;
            block_idx = (block_idx + 1) % (BLOCK_SIZE + 1);
        }
        diff_blocks.push_back(block);

        BOOST_ASSERT(diff_blocks.size() == number_of_blocks &&
                     block_offsets.size() == number_of_blocks);

        sum_lengths = lengths_prefix_sum;
    }

    inline RangeT GetRange(const unsigned id) const
    {
        BOOST_ASSERT(id < block_offsets.size() + diff_blocks.size() * BLOCK_SIZE);
        // internal_idx 0 is implicitly stored in block_offsets[block_idx]
        const unsigned internal_idx = id % (BLOCK_SIZE + 1);
        const unsigned block_idx = id / (BLOCK_SIZE + 1);

        BOOST_ASSERT(block_idx < diff_blocks.size());

        unsigned begin_idx = 0;
        unsigned end_idx = 0;
        begin_idx = block_offsets[block_idx];
        const BlockT &block = diff_blocks[block_idx];
        if (internal_idx > 0)
        {
            begin_idx += PrefixSumAtIndex(internal_idx - 1, block);
        }

        // next index inside current block
        if (internal_idx < BLOCK_SIZE)
        {
            // note internal_idx - 1 is the *current* index for uint8_blocks
            end_idx = begin_idx + block[internal_idx];
        }
        else
        {
            BOOST_ASSERT(block_idx < block_offsets.size() - 1);
            end_idx = block_offsets[block_idx + 1];
        }

        BOOST_ASSERT(begin_idx < sum_lengths && end_idx <= sum_lengths);
        BOOST_ASSERT(begin_idx <= end_idx);

        return osrm::irange(begin_idx, end_idx);
    }

  private:
    inline unsigned PrefixSumAtIndex(int index, const BlockT &block) const;

    // contains offset for each differential block
    OffsetContainerT block_offsets;
    // blocks of differential encoded offsets, should be aligned
    BlockContainerT diff_blocks;
    unsigned sum_lengths;
};

template <unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
unsigned RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY>::PrefixSumAtIndex(int index,
                                                                     const BlockT &block) const
{
    // this loop looks inefficent, but a modern compiler
    // will emit nice SIMD here, at least for sensible block sizes. (I checked.)
    unsigned sum = 0;
    for (int i = 0; i <= index; ++i)
    {
        sum += block[i];
    }

    return sum;
}

template <unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::ostream &operator<<(std::ostream &out, const RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table)
{
    // write number of block
    const unsigned number_of_blocks = table.diff_blocks.size();
    out.write((char *)&number_of_blocks, sizeof(unsigned));
    // write total length
    out.write((char *)&table.sum_lengths, sizeof(unsigned));
    // write block offsets
    out.write((char *)table.block_offsets.data(), sizeof(unsigned) * table.block_offsets.size());
    // write blocks
    out.write((char *)table.diff_blocks.data(), BLOCK_SIZE * table.diff_blocks.size());

    return out;
}

template <unsigned BLOCK_SIZE, bool USE_SHARED_MEMORY>
std::istream &operator>>(std::istream &in, RangeTable<BLOCK_SIZE, USE_SHARED_MEMORY> &table)
{
    // read number of block
    unsigned number_of_blocks;
    in.read((char *)&number_of_blocks, sizeof(unsigned));
    // read total length
    in.read((char *)&table.sum_lengths, sizeof(unsigned));

    table.block_offsets.resize(number_of_blocks);
    table.diff_blocks.resize(number_of_blocks);

    // read block offsets
    in.read((char *)table.block_offsets.data(), sizeof(unsigned) * number_of_blocks);
    // read blocks
    in.read((char *)table.diff_blocks.data(), BLOCK_SIZE * number_of_blocks);
    return in;
}

#endif // RANGE_TABLE_HPP
