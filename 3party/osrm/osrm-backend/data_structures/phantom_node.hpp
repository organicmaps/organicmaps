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

#ifndef PHANTOM_NODES_H
#define PHANTOM_NODES_H

#include "travel_mode.hpp"
#include "../typedefs.h"

#include "../include/osrm/coordinate.hpp"

#include <iostream>
#include <utility>
#include <vector>

struct PhantomNode
{
    PhantomNode(NodeID forward_node_id,
                NodeID reverse_node_id,
                unsigned name_id,
                int forward_weight,
                int reverse_weight,
                int forward_offset,
                int reverse_offset,
                unsigned packed_geometry_id,
                unsigned component_id,
                FixedPointCoordinate &location,
                unsigned short fwd_segment_position,
                TravelMode forward_travel_mode,
                TravelMode backward_travel_mode);

    PhantomNode();

    template <class OtherT> PhantomNode(const OtherT &other, const FixedPointCoordinate &foot_point)
    {
        forward_node_id = other.forward_edge_based_node_id;
        reverse_node_id = other.reverse_edge_based_node_id;
        name_id = other.name_id;

        forward_weight = other.forward_weight;
        reverse_weight = other.reverse_weight;

        forward_offset = other.forward_offset;
        reverse_offset = other.reverse_offset;

        packed_geometry_id = other.packed_geometry_id;
        component_id = other.component_id;

        location = foot_point;
        fwd_segment_position = other.fwd_segment_position;

        forward_travel_mode = other.forward_travel_mode;
        backward_travel_mode = other.backward_travel_mode;
    }

    NodeID forward_node_id;
    NodeID reverse_node_id;
    unsigned name_id;
    int forward_weight;
    int reverse_weight;
    int forward_offset;
    int reverse_offset;
    unsigned packed_geometry_id;
    unsigned component_id;
    FixedPointCoordinate location;
    unsigned short fwd_segment_position;
    TravelMode forward_travel_mode : 4;
    TravelMode backward_travel_mode : 4;

    int GetForwardWeightPlusOffset() const;

    int GetReverseWeightPlusOffset() const;

    bool is_bidirected() const;

    bool is_compressed() const;

    bool is_valid(const unsigned numberOfNodes) const;

    bool is_valid() const;

    bool is_in_tiny_component() const;

    bool operator==(const PhantomNode &other) const;
};

using PhantomNodeArray = std::vector<std::vector<PhantomNode>>;

class phantom_node_pair : public std::pair<PhantomNode, PhantomNode>
{
};

struct PhantomNodeLists
{
    std::vector<PhantomNode> source_phantom_list;
    std::vector<PhantomNode> target_phantom_list;
};

struct PhantomNodes
{
    PhantomNode source_phantom;
    PhantomNode target_phantom;
};

inline std::ostream &operator<<(std::ostream &out, const PhantomNodes &pn)
{
    out << "source_coord: " << pn.source_phantom.location << "\n";
    out << "target_coord: " << pn.target_phantom.location << std::endl;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, const PhantomNode &pn)
{
    out << "node1: " << pn.forward_node_id << ", "
        << "node2: " << pn.reverse_node_id << ", "
        << "name: " << pn.name_id << ", "
        << "fwd-w: " << pn.forward_weight << ", "
        << "rev-w: " << pn.reverse_weight << ", "
        << "fwd-o: " << pn.forward_offset << ", "
        << "rev-o: " << pn.reverse_offset << ", "
        << "geom: " << pn.packed_geometry_id << ", "
        << "comp: " << pn.component_id << ", "
        << "pos: " << pn.fwd_segment_position << ", "
        << "loc: " << pn.location;
    return out;
}

#endif // PHANTOM_NODES_H
