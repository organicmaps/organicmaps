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

#ifndef FIRST_AND_LAST_SEGMENT_OF_WAY_HPP
#define FIRST_AND_LAST_SEGMENT_OF_WAY_HPP

#include "../data_structures/external_memory_node.hpp"
#include "../typedefs.h"

#include <limits>
#include <string>

struct FirstAndLastSegmentOfWay
{
    EdgeID way_id;
    NodeID first_segment_source_id;
    NodeID first_segment_target_id;
    NodeID last_segment_source_id;
    NodeID last_segment_target_id;
    FirstAndLastSegmentOfWay()
        : way_id(std::numeric_limits<EdgeID>::max()),
          first_segment_source_id(std::numeric_limits<NodeID>::max()),
          first_segment_target_id(std::numeric_limits<NodeID>::max()),
          last_segment_source_id(std::numeric_limits<NodeID>::max()),
          last_segment_target_id(std::numeric_limits<NodeID>::max())
    {
    }

    FirstAndLastSegmentOfWay(EdgeID w, NodeID fs, NodeID ft, NodeID ls, NodeID lt)
        : way_id(w), first_segment_source_id(fs), first_segment_target_id(ft),
          last_segment_source_id(ls), last_segment_target_id(lt)
    {
    }

    static FirstAndLastSegmentOfWay min_value()
    {
        return {std::numeric_limits<EdgeID>::min(),
                std::numeric_limits<NodeID>::min(),
                std::numeric_limits<NodeID>::min(),
                std::numeric_limits<NodeID>::min(),
                std::numeric_limits<NodeID>::min()};
    }
    static FirstAndLastSegmentOfWay max_value()
    {
        return {std::numeric_limits<EdgeID>::max(),
                std::numeric_limits<NodeID>::max(),
                std::numeric_limits<NodeID>::max(),
                std::numeric_limits<NodeID>::max(),
                std::numeric_limits<NodeID>::max()};
    }
};

struct FirstAndLastSegmentOfWayStxxlCompare
{
    using value_type = FirstAndLastSegmentOfWay;
    bool operator()(const FirstAndLastSegmentOfWay &a, const FirstAndLastSegmentOfWay &b) const
    {
        return a.way_id < b.way_id;
    }
    value_type max_value() { return FirstAndLastSegmentOfWay::max_value(); }
    value_type min_value() { return FirstAndLastSegmentOfWay::min_value(); }
};

#endif /* FIRST_AND_LAST_SEGMENT_OF_WAY_HPP */
