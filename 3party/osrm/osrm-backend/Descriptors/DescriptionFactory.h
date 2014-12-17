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

#ifndef DESCRIPTIONFACTORY_H_
#define DESCRIPTIONFACTORY_H_

#include "../Algorithms/DouglasPeucker.h"
#include "../Algorithms/PolylineCompressor.h"
#include "../DataStructures/PhantomNodes.h"
#include "../DataStructures/SegmentInformation.h"
#include "../DataStructures/TurnInstructions.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

#include <limits>
#include <vector>

struct PathData;
/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory
{
    DouglasPeucker polyline_generalizer;
    PolylineCompressor polyline_compressor;
    PhantomNode start_phantom, target_phantom;

    double DegreeToRadian(const double degree) const;
    double RadianToDegree(const double degree) const;

    std::vector<unsigned> via_indices;

  public:
    struct RouteSummary
    {
        unsigned distance;
        EdgeWeight duration;
        unsigned source_name_id;
        unsigned target_name_id;
        RouteSummary() : distance(0), duration(0), source_name_id(0), target_name_id(0) {}

        void BuildDurationAndLengthStrings(const double raw_distance, const unsigned raw_duration)
        {
            // compute distance/duration for route summary
            distance = static_cast<unsigned>(round(raw_distance));
            duration = static_cast<unsigned>(round(raw_duration / 10.));
        }
    } summary;

    double entireLength;

    // I know, declaring this public is considered bad. I'm lazy
    std::vector<SegmentInformation> path_description;
    DescriptionFactory();
    void AppendSegment(const FixedPointCoordinate &coordinate, const PathData &data);
    void BuildRouteSummary(const double distance, const unsigned time);
    void SetStartSegment(const PhantomNode &start_phantom, const bool traversed_in_reverse);
    void SetEndSegment(const PhantomNode &start_phantom,
                       const bool traversed_in_reverse,
                       const bool is_via_location = false);
    JSON::Value AppendGeometryString(const bool return_encoded);
    std::vector<unsigned> const &GetViaIndices() const;

    template <class DataFacadeT> void Run(const DataFacadeT *facade, const unsigned zoomLevel)
    {
        if (path_description.empty())
        {
            return;
        }

        /** starts at index 1 */
        path_description[0].length = 0;
        for (unsigned i = 1; i < path_description.size(); ++i)
        {
            // move down names by one, q&d hack
            path_description[i - 1].name_id = path_description[i].name_id;
            path_description[i].length = FixedPointCoordinate::ApproximateEuclideanDistance(
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
        unsigned segment_duration = 0;
        unsigned segment_start_index = 0;

        for (unsigned i = 1; i < path_description.size(); ++i)
        {
            entireLength += path_description[i].length;
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
        if (std::numeric_limits<double>::epsilon() > path_description.back().length)
        {
            if (path_description.size() > 2)
            {
                path_description.pop_back();
                path_description.back().necessary = true;
                path_description.back().turn_instruction = TurnInstruction::NoTurn;
                target_phantom.name_id = (path_description.end() - 2)->name_id;
            }
        }
        if (std::numeric_limits<double>::epsilon() > path_description.front().length)
        {
            if (path_description.size() > 2)
            {
                path_description.erase(path_description.begin());
                path_description.front().turn_instruction = TurnInstruction::HeadOn;
                path_description.front().necessary = true;
                start_phantom.name_id = path_description.front().name_id;
            }
        }

        // Generalize poly line
        polyline_generalizer.Run(path_description, zoomLevel);

        // fix what needs to be fixed else
        unsigned necessary_pieces = 0; // a running index that counts the necessary pieces
        for (unsigned i = 0; i < path_description.size() - 1 && path_description.size() >= 2; ++i)
        {
            if (path_description[i].necessary)
            {
                ++necessary_pieces;
                if (path_description[i].is_via_location)
                { // mark the end of a leg
                    via_indices.push_back(necessary_pieces);
                }
                const double angle =
                    path_description[i + 1].location.GetBearing(path_description[i].location);
                path_description[i].bearing = static_cast<unsigned>(angle * 10);
            }
        }
        via_indices.push_back(necessary_pieces + 1);
        BOOST_ASSERT(via_indices.size() >= 2);
        // BOOST_ASSERT(0 != necessary_pieces || path_description.empty());
        return;
    }
};

#endif /* DESCRIPTIONFACTORY_H_ */
