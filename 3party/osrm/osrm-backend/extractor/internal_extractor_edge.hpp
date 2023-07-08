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

#ifndef INTERNAL_EXTRACTOR_EDGE_HPP
#define INTERNAL_EXTRACTOR_EDGE_HPP

#include "../typedefs.h"
#include "../data_structures/travel_mode.hpp"

#include <boost/assert.hpp>

#include <osrm/coordinate.hpp>

struct InternalExtractorEdge
{
    InternalExtractorEdge()
        : way_id(0), start(0), target(0), speed(0), name_id(0), direction(0), is_roundabout(false),
          is_in_tiny_cc(false), is_duration_set(false), is_access_restricted(false),
          is_split(false), travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    explicit InternalExtractorEdge(unsigned id,
                                   NodeID start,
                                   NodeID target,
                                   short direction,
                                   double speed,
                                   unsigned name_id,
                                   bool is_roundabout,
                                   bool is_in_tiny_cc,
                                   bool is_duration_set,
                                   bool is_access_restricted,
                                   TravelMode travel_mode,
                                   bool is_split)
        : way_id(id), start(start), target(target), speed(speed), name_id(name_id), direction(direction),
          is_roundabout(is_roundabout), is_in_tiny_cc(is_in_tiny_cc),
          is_duration_set(is_duration_set), is_access_restricted(is_access_restricted),
          is_split(is_split), travel_mode(travel_mode)
    {
    }

    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_value()
    {
        return InternalExtractorEdge(0, 0, 0, 0, 0, 0, false, false, false, false,
                                     TRAVEL_MODE_INACCESSIBLE, false);
    }
    static InternalExtractorEdge max_value()
    {
        return InternalExtractorEdge(SPECIAL_NODEID, SPECIAL_NODEID, SPECIAL_NODEID, 0, 0, 0, false, false, false,
                                     false, TRAVEL_MODE_INACCESSIBLE, false);
    }

    unsigned way_id;
    NodeID start;
    NodeID target;
    double speed;
    unsigned name_id;
    short direction;
    bool is_roundabout : 1;
    bool is_in_tiny_cc : 1;
    bool is_duration_set : 1;
    bool is_access_restricted : 1;
    bool is_split : 1;
    TravelMode travel_mode : 4;

    FixedPointCoordinate source_coordinate;
    FixedPointCoordinate target_coordinate;
};

struct CmpEdgeByStartID
{
    using value_type = InternalExtractorEdge;
    bool operator()(const InternalExtractorEdge &a, const InternalExtractorEdge &b) const
    {
        return a.start < b.start;
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }

    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

struct CmpEdgeByTargetID
{
    using value_type = InternalExtractorEdge;

    bool operator()(const InternalExtractorEdge &a, const InternalExtractorEdge &b) const
    {
        return a.target < b.target;
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }

    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

#endif // INTERNAL_EXTRACTOR_EDGE_HPP
