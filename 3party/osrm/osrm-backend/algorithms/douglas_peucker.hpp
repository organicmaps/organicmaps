/*

Copyright (c) 2013, Project OSRM contributors
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

#ifndef DOUGLAS_PEUCKER_HPP_
#define DOUGLAS_PEUCKER_HPP_

#include "../data_structures/segment_information.hpp"

#include <array>
#include <stack>
#include <utility>
#include <vector>

/* This class object computes the bitvector of indicating generalized input
 * points according to the (Ramer-)Douglas-Peucker algorithm.
 *
 * Input is vector of pairs. Each pair consists of the point information and a
 * bit indicating if the points is present in the generalization.
 * Note: points may also be pre-selected*/

static const std::array<int, 19> DOUGLAS_PEUCKER_THRESHOLDS{{
    512440, // z0
    256720, // z1
    122560, // z2
    56780,  // z3
    28800,  // z4
    14400,  // z5
    7200,   // z6
    3200,   // z7
    2400,   // z8
    1000,   // z9
    600,    // z10
    120,    // z11
    60,     // z12
    45,     // z13
    36,     // z14
    20,     // z15
    8,      // z16
    6,      // z17
    4       // z18
}};

class DouglasPeucker
{
  public:
    using RandomAccessIt = std::vector<SegmentInformation>::iterator;

    using GeometryRange = std::pair<RandomAccessIt, RandomAccessIt>;
    // Stack to simulate the recursion
    std::stack<GeometryRange> recursion_stack;

  public:
    void Run(RandomAccessIt begin, RandomAccessIt end, const unsigned zoom_level);
    void Run(std::vector<SegmentInformation> &input_geometry, const unsigned zoom_level);
};

#endif /* DOUGLAS_PEUCKER_HPP_ */
