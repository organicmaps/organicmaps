/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#ifndef DOUGLASPEUCKER_H_
#define DOUGLASPEUCKER_H_

#include <stack>
#include <utility>
#include <vector>

/* This class object computes the bitvector of indicating generalized input
 * points according to the (Ramer-)Douglas-Peucker algorithm.
 *
 * Input is vector of pairs. Each pair consists of the point information and a
 * bit indicating if the points is present in the generalization.
 * Note: points may also be pre-selected*/

struct SegmentInformation;

class DouglasPeucker
{
  private:
    std::vector<int> douglas_peucker_thresholds;

    using GeometryRange = std::pair<unsigned, unsigned>;
    // Stack to simulate the recursion
    std::stack<GeometryRange> recursion_stack;

  public:
    DouglasPeucker();
    void Run(std::vector<SegmentInformation> &input_geometry, const unsigned zoom_level);
};

#endif /* DOUGLASPEUCKER_H_ */
