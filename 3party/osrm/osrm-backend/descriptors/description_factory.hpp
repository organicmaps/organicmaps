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

#ifndef DESCRIPTION_FACTORY_HPP
#define DESCRIPTION_FACTORY_HPP

#include "../algorithms/douglas_peucker.hpp"
#include "../data_structures/phantom_node.hpp"
#include "../data_structures/segment_information.hpp"
#include "../data_structures/turn_instructions.hpp"

#include <boost/assert.hpp>

#include <osrm/coordinate.hpp>
#include <osrm/json_container.hpp>

#include <cmath>

#include <limits>
#include <vector>

struct PathData;
/* This class is fed with all way segments in consecutive order
 *  and produces the description plus the encoded polyline */

class DescriptionFactory
{
    DouglasPeucker polyline_generalizer;
    PhantomNode start_phantom, target_phantom;

    double DegreeToRadian(const double degree) const;
    double RadianToDegree(const double degree) const;

    std::vector<unsigned> via_indices;

    double entire_length;

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
            distance = static_cast<unsigned>(std::round(raw_distance));
            duration = static_cast<EdgeWeight>(std::round(raw_duration / 10.));
        }
    } summary;

    // I know, declaring this public is considered bad. I'm lazy
    std::vector<SegmentInformation> path_description;
    DescriptionFactory();
    void AppendSegment(const FixedPointCoordinate &coordinate, const PathData &data);
    void BuildRouteSummary(const double distance, const unsigned time);
    void SetStartSegment(const PhantomNode &start_phantom, const bool traversed_in_reverse);
    void SetEndSegment(const PhantomNode &start_phantom,
                       const bool traversed_in_reverse,
                       const bool is_via_location = false);
    osrm::json::Value AppendGeometryString(const bool return_encoded);
    std::vector<unsigned> const &GetViaIndices() const;

    double get_entire_length() const { return entire_length; }

    void Run(const unsigned zoom_level);
};

#endif /* DESCRIPTION_FACTORY_HPP */
