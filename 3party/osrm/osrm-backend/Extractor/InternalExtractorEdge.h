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

#ifndef INTERNAL_EXTRACTOR_EDGE_H
#define INTERNAL_EXTRACTOR_EDGE_H

#include "../typedefs.h"
#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

struct InternalExtractorEdge
{
    InternalExtractorEdge()
        : start(0), target(0), type(0), direction(0), speed(0), name_id(0), is_roundabout(false),
          is_in_tiny_cc(false), is_duration_set(false), is_access_restricted(false),
          is_contra_flow(false), is_split(false)
    {
    }

    explicit InternalExtractorEdge(NodeID start,
                                   NodeID target,
                                   short type,
                                   short direction,
                                   double speed,
                                   unsigned name_id,
                                   bool is_roundabout,
                                   bool is_in_tiny_cc,
                                   bool is_duration_set,
                                   bool is_access_restricted,
                                   bool is_contra_flow,
                                   bool is_split)
        : start(start), target(target), type(type), direction(direction), speed(speed),
          name_id(name_id), is_roundabout(is_roundabout), is_in_tiny_cc(is_in_tiny_cc),
          is_duration_set(is_duration_set), is_access_restricted(is_access_restricted),
          is_contra_flow(is_contra_flow), is_split(is_split)
    {
        BOOST_ASSERT(0 <= type);
    }

    // necessary static util functions for stxxl's sorting
    static InternalExtractorEdge min_value()
    {
        return InternalExtractorEdge(0, 0, 0, 0, 0, 0, false, false, false, false, false, false);
    }
    static InternalExtractorEdge max_value()
    {
        return InternalExtractorEdge(
            SPECIAL_NODEID, SPECIAL_NODEID, 0, 0, 0, 0, false, false, false, false, false, false);
    }

    NodeID start;
    NodeID target;
    short type;
    short direction;
    double speed;
    unsigned name_id;
    bool is_roundabout;
    bool is_in_tiny_cc;
    bool is_duration_set;
    bool is_access_restricted;
    bool is_contra_flow;
    bool is_split;

    FixedPointCoordinate source_coordinate;
    FixedPointCoordinate target_coordinate;
};

struct CmpEdgeByStartID
{
    typedef InternalExtractorEdge value_type;
    bool operator()(const InternalExtractorEdge &a, const InternalExtractorEdge &b) const
    {
        return a.start < b.start;
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }

    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

struct CmpEdgeByTargetID
{
    typedef InternalExtractorEdge value_type;

    bool operator()(const InternalExtractorEdge &a, const InternalExtractorEdge &b) const
    {
        return a.target < b.target;
    }

    value_type max_value() { return InternalExtractorEdge::max_value(); }

    value_type min_value() { return InternalExtractorEdge::min_value(); }
};

#endif // INTERNAL_EXTRACTOR_EDGE_H
