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

#include "DescriptionFactory.h"

#include <osrm/Coordinate.h>

#include "../typedefs.h"
#include "../Algorithms/PolylineCompressor.h"
#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/RawRouteData.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"

DescriptionFactory::DescriptionFactory() : entireLength(0) { via_indices.push_back(0); }

std::vector<unsigned> const &DescriptionFactory::GetViaIndices() const { return via_indices; }

void DescriptionFactory::SetStartSegment(const PhantomNode &source, const bool traversed_in_reverse)
{
    start_phantom = source;
    const EdgeWeight segment_duration =
        (traversed_in_reverse ? source.reverse_weight : source.forward_weight);
    AppendSegment(source.location,
                  PathData(0, source.name_id, TurnInstruction::HeadOn, segment_duration));
    BOOST_ASSERT(path_description.back().duration == segment_duration);
}

void DescriptionFactory::SetEndSegment(const PhantomNode &target, const bool traversed_in_reverse)
{
    target_phantom = target;
    const EdgeWeight segment_duration =
        (traversed_in_reverse ? target.reverse_weight : target.forward_weight);
    path_description.emplace_back(
        target.location, target.name_id, segment_duration, 0.f, TurnInstruction::NoTurn, true, true);
    BOOST_ASSERT(path_description.back().duration == segment_duration);
}

void DescriptionFactory::AppendSegment(const FixedPointCoordinate &coordinate,
                                       const PathData &path_point)
{
    if ((1 == path_description.size()) && (path_description.back().location == coordinate))
    {
        path_description.back().name_id = path_point.name_id;
        return;
    }

    path_description.emplace_back(coordinate,
                                  path_point.name_id,
                                  path_point.segment_duration,
                                  0.f,
                                  path_point.turn_instruction);
}

JSON::Value DescriptionFactory::AppendEncodedPolylineString(const bool return_encoded)
{
    if (return_encoded)
    {
        return polyline_compressor.printEncodedString(path_description);
    }
    return polyline_compressor.printUnencodedString(path_description);
}

void DescriptionFactory::BuildRouteSummary(const double distance, const unsigned time)
{
    summary.source_name_id = start_phantom.name_id;
    summary.target_name_id = target_phantom.name_id;
    summary.BuildDurationAndLengthStrings(distance, time);
}
