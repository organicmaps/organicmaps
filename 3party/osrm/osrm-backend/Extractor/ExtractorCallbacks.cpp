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

#include "ExtractorCallbacks.h"
#include "ExtractionContainers.h"
#include "ExtractionWay.h"

#include "../DataStructures/Restriction.h"
#include "../DataStructures/ImportNode.h"
#include "../Util/simple_logger.hpp"

#include <osrm/Coordinate.h>

#include <limits>
#include <string>
#include <vector>

ExtractorCallbacks::ExtractorCallbacks(ExtractionContainers &extraction_containers,
                                       std::unordered_map<std::string, NodeID> &string_map)
    : string_map(string_map), external_memory(extraction_containers)
{
}

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::ProcessNode(const ExternalMemoryNode &n)
{
    if (n.lat <= 85 * COORDINATE_PRECISION && n.lat >= -85 * COORDINATE_PRECISION)
    {
        external_memory.all_nodes_list.push_back(n);
    }
}

bool ExtractorCallbacks::ProcessRestriction(const InputRestrictionContainer &restriction)
{
    external_memory.restrictions_list.push_back(restriction);
    return true;
}

/** warning: caller needs to take care of synchronization! */
void ExtractorCallbacks::ProcessWay(ExtractionWay &parsed_way)
{
    if (((0 >= parsed_way.forward_speed) ||
            (TRAVEL_MODE_INACCESSIBLE == parsed_way.forward_travel_mode)) &&
        ((0 >= parsed_way.backward_speed) ||
            (TRAVEL_MODE_INACCESSIBLE == parsed_way.backward_travel_mode)) &&
        (0 >= parsed_way.duration))
    { // Only true if the way is specified by the speed profile
        return;
    }

    if (parsed_way.path.size() <= 1)
    { // safe-guard against broken data
        return;
    }

    if (std::numeric_limits<unsigned>::max() == parsed_way.id)
    {
        SimpleLogger().Write(logDEBUG) << "found bogus way with id: " << parsed_way.id
                                       << " of size " << parsed_way.path.size();
        return;
    }

    if (0 < parsed_way.duration)
    {
        // TODO: iterate all way segments and set duration corresponding to the length of each
        // segment
        parsed_way.forward_speed = parsed_way.duration / (parsed_way.path.size() - 1);
        parsed_way.backward_speed = parsed_way.duration / (parsed_way.path.size() - 1);
    }

    if (std::numeric_limits<double>::epsilon() >= std::abs(-1. - parsed_way.forward_speed))
    {
        SimpleLogger().Write(logDEBUG) << "found way with bogus speed, id: " << parsed_way.id;
        return;
    }

    // Get the unique identifier for the street name
    const auto &string_map_iterator = string_map.find(parsed_way.name);
    if (string_map.end() == string_map_iterator)
    {
        parsed_way.nameID = external_memory.name_list.size();
        external_memory.name_list.push_back(parsed_way.name);
        string_map.insert(std::make_pair(parsed_way.name, parsed_way.nameID));
    }
    else
    {
        parsed_way.nameID = string_map_iterator->second;
    }

    if (TRAVEL_MODE_INACCESSIBLE == parsed_way.forward_travel_mode)
    {
        std::reverse(parsed_way.path.begin(), parsed_way.path.end());
        parsed_way.forward_travel_mode = parsed_way.backward_travel_mode;
        parsed_way.backward_travel_mode = TRAVEL_MODE_INACCESSIBLE;
    }

    const bool split_edge =
      (parsed_way.forward_speed>0) && (TRAVEL_MODE_INACCESSIBLE != parsed_way.forward_travel_mode) &&
      (parsed_way.backward_speed>0) && (TRAVEL_MODE_INACCESSIBLE != parsed_way.backward_travel_mode) &&
      ((parsed_way.forward_speed != parsed_way.backward_speed) ||
      (parsed_way.forward_travel_mode != parsed_way.backward_travel_mode));

    BOOST_ASSERT(parsed_way.forward_travel_mode>0);
    for (unsigned n = 0; n < (parsed_way.path.size() - 1); ++n)
    {
        external_memory.all_edges_list.push_back(InternalExtractorEdge(
            parsed_way.path[n],
            parsed_way.path[n + 1],
            ((split_edge || TRAVEL_MODE_INACCESSIBLE == parsed_way.backward_travel_mode) ? ExtractionWay::oneway
                                                                                 : ExtractionWay::bidirectional),
            parsed_way.forward_speed,
            parsed_way.nameID,
            parsed_way.roundabout,
            parsed_way.ignoreInGrid,
            (0 < parsed_way.duration),
            parsed_way.isAccessRestricted,
            parsed_way.forward_travel_mode,
            split_edge));
        external_memory.used_node_id_list.push_back(parsed_way.path[n]);
    }
    external_memory.used_node_id_list.push_back(parsed_way.path.back());

    // The following information is needed to identify start and end segments of restrictions
    external_memory.way_start_end_id_list.push_back(
        WayIDStartAndEndEdge(parsed_way.id,
                             parsed_way.path[0],
                             parsed_way.path[1],
                             parsed_way.path[parsed_way.path.size() - 2],
                             parsed_way.path.back()));

    if (split_edge)
    { // Only true if the way should be split
        BOOST_ASSERT(parsed_way.backward_travel_mode>0);
        std::reverse(parsed_way.path.begin(), parsed_way.path.end());

        for (std::vector<NodeID>::size_type n = 0; n < parsed_way.path.size() - 1; ++n)
        {
            external_memory.all_edges_list.push_back(
                InternalExtractorEdge(parsed_way.path[n],
                                      parsed_way.path[n + 1],
                                      ExtractionWay::oneway,
                                      parsed_way.backward_speed,
                                      parsed_way.nameID,
                                      parsed_way.roundabout,
                                      parsed_way.ignoreInGrid,
                                      (0 < parsed_way.duration),
                                      parsed_way.isAccessRestricted,
                                      parsed_way.backward_travel_mode,
                                      split_edge));
        }
        external_memory.way_start_end_id_list.push_back(
            WayIDStartAndEndEdge(parsed_way.id,
                                 parsed_way.path[0],
                                 parsed_way.path[1],
                                 parsed_way.path[parsed_way.path.size() - 2],
                                 parsed_way.path.back()));
    }
}
