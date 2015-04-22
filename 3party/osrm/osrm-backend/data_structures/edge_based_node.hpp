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

#ifndef EDGE_BASED_NODE_HPP
#define EDGE_BASED_NODE_HPP

#include "../data_structures/travel_mode.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <osrm/coordinate.hpp>

#include <limits>

struct EdgeBasedNode
{
    EdgeBasedNode()
        : forward_edge_based_node_id(SPECIAL_NODEID), reverse_edge_based_node_id(SPECIAL_NODEID),
          u(SPECIAL_NODEID), v(SPECIAL_NODEID), name_id(0),
          forward_weight(INVALID_EDGE_WEIGHT >> 1), reverse_weight(INVALID_EDGE_WEIGHT >> 1),
          forward_offset(0), reverse_offset(0), packed_geometry_id(SPECIAL_EDGEID),
          component_id(-1), fwd_segment_position(std::numeric_limits<unsigned short>::max()),
          forward_travel_mode(TRAVEL_MODE_INACCESSIBLE),
          backward_travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    explicit EdgeBasedNode(NodeID forward_edge_based_node_id,
                           NodeID reverse_edge_based_node_id,
                           NodeID u,
                           NodeID v,
                           unsigned name_id,
                           int forward_weight,
                           int reverse_weight,
                           int forward_offset,
                           int reverse_offset,
                           unsigned packed_geometry_id,
                           unsigned component_id,
                           unsigned short fwd_segment_position,
                           TravelMode forward_travel_mode,
                           TravelMode backward_travel_mode)
        : forward_edge_based_node_id(forward_edge_based_node_id),
          reverse_edge_based_node_id(reverse_edge_based_node_id), u(u), v(v), name_id(name_id),
          forward_weight(forward_weight), reverse_weight(reverse_weight),
          forward_offset(forward_offset), reverse_offset(reverse_offset),
          packed_geometry_id(packed_geometry_id), component_id(component_id),
          fwd_segment_position(fwd_segment_position), forward_travel_mode(forward_travel_mode),
          backward_travel_mode(backward_travel_mode)
    {
        BOOST_ASSERT((forward_edge_based_node_id != SPECIAL_NODEID) ||
                     (reverse_edge_based_node_id != SPECIAL_NODEID));
    }

    static inline FixedPointCoordinate Centroid(const FixedPointCoordinate &a,
                                                const FixedPointCoordinate &b)
    {
        FixedPointCoordinate centroid;
        // The coordinates of the midpoint are given by:
        centroid.lat = (a.lat + b.lat) / 2;
        centroid.lon = (a.lon + b.lon) / 2;
        return centroid;
    }

    bool IsCompressed() const { return packed_geometry_id != SPECIAL_EDGEID; }

    bool is_in_tiny_cc() const { return 0 != component_id; }

    NodeID forward_edge_based_node_id; // needed for edge-expanded graph
    NodeID reverse_edge_based_node_id; // needed for edge-expanded graph
    NodeID u;                          // indices into the coordinates array
    NodeID v;                          // indices into the coordinates array
    unsigned name_id;                  // id of the edge name
    int forward_weight;                // weight of the edge
    int reverse_weight;                // weight in the other direction (may be different)
    int forward_offset;          // prefix sum of the weight up the edge TODO: short must suffice
    int reverse_offset;          // prefix sum of the weight from the edge TODO: short must suffice
    unsigned packed_geometry_id; // if set, then the edge represents a packed geometry
    unsigned component_id;
    unsigned short fwd_segment_position; // segment id in a compressed geometry
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;
};

#endif // EDGE_BASED_NODE_HPP
