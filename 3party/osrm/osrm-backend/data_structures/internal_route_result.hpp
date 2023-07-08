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

#ifndef RAW_ROUTE_DATA_H
#define RAW_ROUTE_DATA_H

#include "../data_structures/phantom_node.hpp"
#include "../data_structures/travel_mode.hpp"
#include "../data_structures/turn_instructions.hpp"
#include "../typedefs.h"

#include <osrm/coordinate.hpp>

#include <vector>

struct PathData
{
    PathData()
        : node(SPECIAL_NODEID), name_id(INVALID_EDGE_WEIGHT), segment_duration(INVALID_EDGE_WEIGHT),
          turn_instruction(TurnInstruction::NoTurn), travel_mode(TRAVEL_MODE_INACCESSIBLE)
    {
    }

    PathData(NodeID node,
             unsigned name_id,
             TurnInstruction turn_instruction,
             EdgeWeight segment_duration,
             TravelMode travel_mode)
        : node(node), name_id(name_id), segment_duration(segment_duration),
          turn_instruction(turn_instruction), travel_mode(travel_mode)
    {
    }
    NodeID node;
    unsigned name_id;
    EdgeWeight segment_duration;
    TurnInstruction turn_instruction;
    TravelMode travel_mode : 4;
};

struct InternalRouteResult
{
    std::vector<std::vector<PathData>> unpacked_path_segments;
    std::vector<PathData> unpacked_alternative;
    std::vector<PhantomNodes> segment_end_coordinates;
    std::vector<bool> source_traversed_in_reverse;
    std::vector<bool> target_traversed_in_reverse;
    std::vector<bool> alt_source_traversed_in_reverse;
    std::vector<bool> alt_target_traversed_in_reverse;
    int shortest_path_length;
    int alternative_path_length;

    bool is_via_leg(const std::size_t leg) const
    {
        return (leg != unpacked_path_segments.size() - 1);
    }

    InternalRouteResult()
        : shortest_path_length(INVALID_EDGE_WEIGHT), alternative_path_length(INVALID_EDGE_WEIGHT)
    {
    }
};

#endif // RAW_ROUTE_DATA_H
