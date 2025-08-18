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

#include "description_factory.hpp"

#include "../algorithms/polyline_formatter.hpp"
#include "../data_structures/coordinate_calculation.hpp"
#include "../data_structures/internal_route_result.hpp"
#include "../data_structures/turn_instructions.hpp"
#include "../util/container.hpp"
#include "../util/integer_range.hpp"
#include "../typedefs.h"

DescriptionFactory::DescriptionFactory() : entire_length(0) { via_indices.push_back(0); }

std::vector<unsigned> const &DescriptionFactory::GetViaIndices() const { return via_indices; }

void DescriptionFactory::SetStartSegment(const PhantomNode &source, const bool traversed_in_reverse)
{
    start_phantom = source;
    const EdgeWeight segment_duration =
        (traversed_in_reverse ? source.reverse_weight : source.forward_weight);
    const TravelMode travel_mode =
        (traversed_in_reverse ? source.backward_travel_mode : source.forward_travel_mode);
    AppendSegment(source.location, PathData(0, source.name_id, TurnInstruction::HeadOn,
                                            segment_duration, travel_mode));
    BOOST_ASSERT(path_description.back().duration == segment_duration);
}

void DescriptionFactory::SetEndSegment(const PhantomNode &target,
                                       const bool traversed_in_reverse,
                                       const bool is_via_location)
{
    target_phantom = target;
    const EdgeWeight segment_duration =
        (traversed_in_reverse ? target.reverse_weight : target.forward_weight);
    const TravelMode travel_mode =
        (traversed_in_reverse ? target.backward_travel_mode : target.forward_travel_mode);
    path_description.emplace_back(target.location, target.name_id, segment_duration, 0.f,
                                  is_via_location ? TurnInstruction::ReachViaLocation
                                                  : TurnInstruction::NoTurn,
                                  true, true, travel_mode);
    BOOST_ASSERT(path_description.back().duration == segment_duration);
}

void DescriptionFactory::AppendSegment(const FixedPointCoordinate &coordinate,
                                       const PathData &path_point)
{
    // if the start location is on top of a node, the first movement might be zero-length,
    // in which case we dont' add a new description, but instead update the existing one
    if ((1 == path_description.size()) && (path_description.front().location == coordinate))
    {
        if (path_point.segment_duration > 0)
        {
            path_description.front().name_id = path_point.name_id;
            path_description.front().travel_mode = path_point.travel_mode;
        }
        return;
    }

    // make sure mode changes are announced, even when there otherwise is no turn
    const TurnInstruction turn = [&]() -> TurnInstruction
    {
        if (TurnInstruction::NoTurn == path_point.turn_instruction &&
            path_description.front().travel_mode != path_point.travel_mode &&
            path_point.segment_duration > 0)
        {
            return TurnInstruction::GoStraight;
        }
        return path_point.turn_instruction;
    }();

    path_description.emplace_back(coordinate, path_point.name_id, path_point.segment_duration, 0.f,
                                  turn, path_point.travel_mode);
}

osrm::json::Value DescriptionFactory::AppendGeometryString(const bool return_encoded)
{
    if (return_encoded)
    {
        return PolylineFormatter().printEncodedString(path_description);
    }
    return PolylineFormatter().printUnencodedString(path_description);
}

void DescriptionFactory::BuildRouteSummary(const double distance, const unsigned time)
{
    summary.source_name_id = start_phantom.name_id;
    summary.target_name_id = target_phantom.name_id;
    summary.BuildDurationAndLengthStrings(distance, time);
}

void DescriptionFactory::Run(const unsigned zoom_level)
{
    if (path_description.empty())
    {
        return;
    }

    /** starts at index 1 */
    path_description[0].length = 0.f;
    for (const auto i : osrm::irange<std::size_t>(1, path_description.size()))
    {
        // move down names by one, q&d hack
        path_description[i - 1].name_id = path_description[i].name_id;
        path_description[i].length = coordinate_calculation::euclidean_distance(
            path_description[i - 1].location, path_description[i].location);
    }

    /*Simplify turn instructions
    Input :
    10. Turn left on B 36 for 20 km
    11. Continue on B 35; B 36 for 2 km
    12. Continue on B 36 for 13 km

    becomes:
    10. Turn left on B 36 for 35 km
    */
    // TODO: rework to check only end and start of string.
    //      stl string is way to expensive

    //    unsigned lastTurn = 0;
    //    for(unsigned i = 1; i < path_description.size(); ++i) {
    //        string1 = sEngine.GetEscapedNameForNameID(path_description[i].name_id);
    //        if(TurnInstruction::GoStraight == path_description[i].turn_instruction) {
    //            if(std::string::npos != string0.find(string1+";")
    //                  || std::string::npos != string0.find(";"+string1)
    //                  || std::string::npos != string0.find(string1+" ;")
    //                    || std::string::npos != string0.find("; "+string1)
    //                    ){
    //                SimpleLogger().Write() << "->next correct: " << string0 << " contains " <<
    //                string1;
    //                for(; lastTurn != i; ++lastTurn)
    //                    path_description[lastTurn].name_id = path_description[i].name_id;
    //                path_description[i].turn_instruction = TurnInstruction::NoTurn;
    //            } else if(std::string::npos != string1.find(string0+";")
    //                  || std::string::npos != string1.find(";"+string0)
    //                    || std::string::npos != string1.find(string0+" ;")
    //                    || std::string::npos != string1.find("; "+string0)
    //                    ){
    //                SimpleLogger().Write() << "->prev correct: " << string1 << " contains " <<
    //                string0;
    //                path_description[i].name_id = path_description[i-1].name_id;
    //                path_description[i].turn_instruction = TurnInstruction::NoTurn;
    //            }
    //        }
    //        if (TurnInstruction::NoTurn != path_description[i].turn_instruction) {
    //            lastTurn = i;
    //        }
    //        string0 = string1;
    //    }

    float segment_length = 0.;
    EdgeWeight segment_duration = 0;
    std::size_t segment_start_index = 0;

    for (const auto i : osrm::irange<std::size_t>(1, path_description.size()))
    {
        entire_length += path_description[i].length;
        segment_length += path_description[i].length;
        segment_duration += path_description[i].duration;
        path_description[segment_start_index].length = segment_length;
        path_description[segment_start_index].duration = segment_duration;

        if (TurnInstruction::NoTurn != path_description[i].turn_instruction)
        {
            BOOST_ASSERT(path_description[i].necessary);
            segment_length = 0;
            segment_duration = 0;
            segment_start_index = i;
        }
    }

    // Post-processing to remove empty or nearly empty path segments
    if (path_description.size() > 2 &&
        std::numeric_limits<float>::epsilon() > path_description.back().length)
    {
        path_description.pop_back();
        path_description.back().necessary = true;
        path_description.back().turn_instruction = TurnInstruction::NoTurn;
        target_phantom.name_id = (path_description.end() - 2)->name_id;
    }

    if (path_description.size() > 2 &&
        std::numeric_limits<float>::epsilon() > path_description.front().length)
    {
        path_description.erase(path_description.begin());
        path_description.front().turn_instruction = TurnInstruction::HeadOn;
        path_description.front().necessary = true;
        start_phantom.name_id = path_description.front().name_id;
    }

    // Generalize poly line
    polyline_generalizer.Run(path_description.begin(), path_description.end(), zoom_level);

    // fix what needs to be fixed else
    unsigned necessary_segments = 0; // a running index that counts the necessary pieces
    osrm::for_each_pair(
        path_description, [&](SegmentInformation &first, const SegmentInformation &second)
        {
            if (!first.necessary)
            {
                return;
            }

            ++necessary_segments;

            if (first.is_via_location)
            { // mark the end of a leg (of several segments)
                via_indices.push_back(necessary_segments);
            }

            const double angle = coordinate_calculation::bearing(first.location, second.location);
            first.bearing = static_cast<short>(angle * 10);
        });

    via_indices.push_back(necessary_segments + 1);
    BOOST_ASSERT(via_indices.size() >= 2);
    return;
}
