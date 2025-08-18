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

#include "import_edge.hpp"

#include "travel_mode.hpp"
#include "../typedefs.h"

bool NodeBasedEdge::operator<(const NodeBasedEdge &other) const
{
    if (source == other.source)
    {
        if (target == other.target)
        {
            if (weight == other.weight)
            {
                return forward && backward && ((!other.forward) || (!other.backward));
            }
            return weight < other.weight;
        }
        return target < other.target;
    }
    return source < other.source;
}

NodeBasedEdge::NodeBasedEdge(unsigned way_id,
                             NodeID source,
                             NodeID target,
                             NodeID name_id,
                             EdgeWeight weight,
                             bool forward,
                             bool backward,
                             bool roundabout,
                             bool in_tiny_cc,
                             bool access_restricted,
                             TravelMode travel_mode,
                             bool is_split)
    : way_id(way_id), source(source), target(target), name_id(name_id), weight(weight), forward(forward),
      backward(backward), roundabout(roundabout), in_tiny_cc(in_tiny_cc),
      access_restricted(access_restricted), is_split(is_split), travel_mode(travel_mode)
{
}

bool EdgeBasedEdge::operator<(const EdgeBasedEdge &other) const
{
    if (source == other.source)
    {
        if (target == other.target)
        {
            if (weight == other.weight)
            {
                return forward && backward && ((!other.forward) || (!other.backward));
            }
            return weight < other.weight;
        }
        return target < other.target;
    }
    return source < other.source;
}

template <class EdgeT>
EdgeBasedEdge::EdgeBasedEdge(const EdgeT &other)
    : source(other.source), target(other.target), edge_id(other.data.via),
      weight(other.data.distance), forward(other.data.forward), backward(other.data.backward)
{
}

/** Default constructor. target and weight are set to 0.*/
EdgeBasedEdge::EdgeBasedEdge()
    : source(0), target(0), edge_id(0), weight(0), forward(false), backward(false)
{
}

EdgeBasedEdge::EdgeBasedEdge(const NodeID source,
                             const NodeID target,
                             const NodeID edge_id,
                             const EdgeWeight weight,
                             const bool forward,
                             const bool backward)
    : source(source), target(target), edge_id(edge_id), weight(weight), forward(forward),
      backward(backward)
{
}
