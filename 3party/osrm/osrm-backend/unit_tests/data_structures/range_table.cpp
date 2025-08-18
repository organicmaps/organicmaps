/*

Copyright (c) 2014, Project OSRM contributors
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

#include "../../data_structures/range_table.hpp"
#include "../../typedefs.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <numeric>

constexpr unsigned BLOCK_SIZE = 16;
typedef RangeTable<BLOCK_SIZE, false> TestRangeTable;

BOOST_AUTO_TEST_SUITE(range_table)

void ConstructionTest(std::vector<unsigned> lengths, std::vector<unsigned> offsets)
{
    BOOST_ASSERT(lengths.size() == offsets.size() - 1);

    TestRangeTable table(lengths);

    for (unsigned i = 0; i < lengths.size(); i++)
    {
        auto range = table.GetRange(i);
        BOOST_CHECK_EQUAL(range.front(), offsets[i]);
        BOOST_CHECK_EQUAL(range.back() + 1, offsets[i + 1]);
    }
}

void ComputeLengthsOffsets(std::vector<unsigned> &lengths,
                           std::vector<unsigned> &offsets,
                           unsigned num)
{
    lengths.resize(num);
    offsets.resize(num + 1);
    std::iota(lengths.begin(), lengths.end(), 1);
    offsets[0] = 0;
    std::partial_sum(lengths.begin(), lengths.end(), offsets.begin() + 1);

    std::stringstream l_ss;
    l_ss << "Lengths: ";
    for (auto l : lengths)
        l_ss << l << ", ";
    BOOST_TEST_MESSAGE(l_ss.str());
    std::stringstream o_ss;
    o_ss << "Offsets: ";
    for (auto o : offsets)
        o_ss << o << ", ";
    BOOST_TEST_MESSAGE(o_ss.str());
}

BOOST_AUTO_TEST_CASE(serialization_test)
{
    std::vector<unsigned> lengths;
    std::vector<unsigned> offsets;
    ComputeLengthsOffsets(lengths, offsets, (BLOCK_SIZE + 1) * 10);

    TestRangeTable in_table(lengths);
    TestRangeTable out_table;

    std::stringstream ss;
    ss << in_table;
    ss >> out_table;

    for (unsigned i = 0; i < lengths.size(); i++)
    {
        auto range = out_table.GetRange(i);
        BOOST_CHECK_EQUAL(range.front(), offsets[i]);
        BOOST_CHECK_EQUAL(range.back() + 1, offsets[i + 1]);
    }
}

BOOST_AUTO_TEST_CASE(construction_test)
{
    // only offset empty block
    ConstructionTest({1}, {0, 1});
    // first block almost full => sentinel is last element of block
    // [0] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, (16)}
    std::vector<unsigned> almost_full_lengths;
    std::vector<unsigned> almost_full_offsets;
    ComputeLengthsOffsets(almost_full_lengths, almost_full_offsets, BLOCK_SIZE);
    ConstructionTest(almost_full_lengths, almost_full_offsets);

    // first block full => sentinel is offset of new block, next block empty
    // [0]     {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
    // [(153)] {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    std::vector<unsigned> full_lengths;
    std::vector<unsigned> full_offsets;
    ComputeLengthsOffsets(full_lengths, full_offsets, BLOCK_SIZE + 1);
    ConstructionTest(full_lengths, full_offsets);

    // first block full and offset of next block not sentinel, but the first differential value
    // [0]   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
    // [153] {(17), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    std::vector<unsigned> over_full_lengths;
    std::vector<unsigned> over_full_offsets;
    ComputeLengthsOffsets(over_full_lengths, over_full_offsets, BLOCK_SIZE + 2);
    ConstructionTest(over_full_lengths, over_full_offsets);

    // test multiple blocks
    std::vector<unsigned> multiple_lengths;
    std::vector<unsigned> multiple_offsets;
    ComputeLengthsOffsets(multiple_lengths, multiple_offsets, (BLOCK_SIZE + 1) * 10);
    ConstructionTest(multiple_lengths, multiple_offsets);
}

BOOST_AUTO_TEST_SUITE_END()
