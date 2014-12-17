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

#include "DouglasPeucker.h"

#include "../DataStructures/Range.h"
#include "../DataStructures/SegmentInformation.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

#include <cmath>

#include <algorithm>

namespace {
struct CoordinatePairCalculator
{
    CoordinatePairCalculator() = delete;
    CoordinatePairCalculator(const FixedPointCoordinate &coordinate_a,
                             const FixedPointCoordinate &coordinate_b)
    {
        // initialize distance calculator with two fixed coordinates a, b
        const float RAD = 0.017453292519943295769236907684886f;
        first_lat = (coordinate_a.lat / COORDINATE_PRECISION) * RAD;
        first_lon = (coordinate_a.lon / COORDINATE_PRECISION) * RAD;
        second_lat = (coordinate_b.lat / COORDINATE_PRECISION) * RAD;
        second_lon = (coordinate_b.lon / COORDINATE_PRECISION) * RAD;
    }

    int operator()(FixedPointCoordinate &other) const
    {
        // set third coordinate c
        const float RAD = 0.017453292519943295769236907684886f;
        const float earth_radius = 6372797.560856f;
        const float float_lat1 = (other.lat / COORDINATE_PRECISION) * RAD;
        const float float_lon1 = (other.lon / COORDINATE_PRECISION) * RAD;

        // compute distance (a,c)
        const float x_value_1 = (first_lon - float_lon1) * cos((float_lat1 + first_lat) / 2.f);
        const float y_value_1 = first_lat - float_lat1;
        const float dist1 = sqrt(std::pow(x_value_1, 2) + std::pow(y_value_1, 2)) * earth_radius;

        // compute distance (b,c)
        const float x_value_2 = (second_lon - float_lon1) * cos((float_lat1 + second_lat) / 2.f);
        const float y_value_2 = second_lat - float_lat1;
        const float dist2 = sqrt(std::pow(x_value_2, 2) + std::pow(y_value_2, 2)) * earth_radius;

        // return the minimum
        return static_cast<int>(std::min(dist1, dist2));
    }

    float first_lat;
    float first_lon;
    float second_lat;
    float second_lon;
};
}

DouglasPeucker::DouglasPeucker()
    : douglas_peucker_thresholds({512440, // z0
                                  256720, // z1
                                  122560, // z2
                                  56780,  // z3
                                  28800,  // z4
                                  14400,  // z5
                                  7200,   // z6
                                  3200,   // z7
                                  2400,   // z8
                                  1000,   // z9
                                  600,    // z10
                                  120,    // z11
                                  60,     // z12
                                  45,     // z13
                                  36,     // z14
                                  20,     // z15
                                  8,      // z16
                                  6,      // z17
                                  4       // z18
      })
{
}

void DouglasPeucker::Run(std::vector<SegmentInformation> &input_geometry, const unsigned zoom_level)
{
    // check if input data is invalid
    BOOST_ASSERT_MSG(!input_geometry.empty(), "geometry invalid");

    if (input_geometry.size() < 2)
    {
        return;
    }

    input_geometry.front().necessary = true;
    input_geometry.back().necessary = true;

    {
        BOOST_ASSERT_MSG(zoom_level < 19, "unsupported zoom level");
        unsigned left_border = 0;
        unsigned right_border = 1;
        // Sweep over array and identify those ranges that need to be checked
        do
        {
            // traverse list until new border element found
            if (input_geometry[right_border].necessary)
            {
                // sanity checks
                BOOST_ASSERT(input_geometry[left_border].necessary);
                BOOST_ASSERT(input_geometry[right_border].necessary);
                recursion_stack.emplace(left_border, right_border);
                left_border = right_border;
            }
            ++right_border;
        } while (right_border < input_geometry.size());
    }

    // mark locations as 'necessary' by divide-and-conquer
    while (!recursion_stack.empty())
    {
        // pop next element
        const GeometryRange pair = recursion_stack.top();
        recursion_stack.pop();
        // sanity checks
        BOOST_ASSERT_MSG(input_geometry[pair.first].necessary, "left border mus be necessary");
        BOOST_ASSERT_MSG(input_geometry[pair.second].necessary, "right border must be necessary");
        BOOST_ASSERT_MSG(pair.second < input_geometry.size(), "right border outside of geometry");
        BOOST_ASSERT_MSG(pair.first < pair.second, "left border on the wrong side");

        int max_int_distance = 0;
        unsigned farthest_entry_index = pair.second;
        const CoordinatePairCalculator dist_calc(input_geometry[pair.first].location,
                                                 input_geometry[pair.second].location);

        // sweep over range to find the maximum
        for (const auto i : osrm::irange(pair.first + 1, pair.second))
        {
            const int distance = dist_calc(input_geometry[i].location);
            // found new feasible maximum?
            if (distance > max_int_distance && distance > douglas_peucker_thresholds[zoom_level])
            {
                farthest_entry_index = i;
                max_int_distance = distance;
            }
        }

        // check if maximum violates a zoom level dependent threshold
        if (max_int_distance > douglas_peucker_thresholds[zoom_level])
        {
            //  mark idx as necessary
            input_geometry[farthest_entry_index].necessary = true;
            if (1 < (farthest_entry_index - pair.first))
            {
                recursion_stack.emplace(pair.first, farthest_entry_index);
            }
            if (1 < (pair.second - farthest_entry_index))
            {
                recursion_stack.emplace(farthest_entry_index, pair.second);
            }
        }
    }
}
