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

#include "douglas_peucker.hpp"

#include "../data_structures/segment_information.hpp"

#include <boost/assert.hpp>
#include <osrm/coordinate.hpp>

#include <cmath>
#include <algorithm>
#include <iterator>

namespace
{
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
        const float dist1 = std::hypot(x_value_1, y_value_1) * earth_radius;

        // compute distance (b,c)
        const float x_value_2 = (second_lon - float_lon1) * cos((float_lat1 + second_lat) / 2.f);
        const float y_value_2 = second_lat - float_lat1;
        const float dist2 = std::hypot(x_value_2, y_value_2) * earth_radius;

        // return the minimum
        return static_cast<int>(std::min(dist1, dist2));
    }

    float first_lat;
    float first_lon;
    float second_lat;
    float second_lon;
};
}

void DouglasPeucker::Run(std::vector<SegmentInformation> &input_geometry, const unsigned zoom_level)
{
    Run(std::begin(input_geometry), std::end(input_geometry), zoom_level);
}

void DouglasPeucker::Run(RandomAccessIt begin, RandomAccessIt end, const unsigned zoom_level)
{
    const auto size = std::distance(begin, end);
    if (size < 2)
    {
        return;
    }

    begin->necessary = true;
    std::prev(end)->necessary = true;

    {
        BOOST_ASSERT_MSG(zoom_level < DOUGLAS_PEUCKER_THRESHOLDS.size(), "unsupported zoom level");
        RandomAccessIt left_border = begin;
        RandomAccessIt right_border = std::next(begin);
        // Sweep over array and identify those ranges that need to be checked
        do
        {
            // traverse list until new border element found
            if (right_border->necessary)
            {
                // sanity checks
                BOOST_ASSERT(left_border->necessary);
                BOOST_ASSERT(right_border->necessary);
                recursion_stack.emplace(left_border, right_border);
                left_border = right_border;
            }
            ++right_border;
        } while (right_border != end);
    }

    // mark locations as 'necessary' by divide-and-conquer
    while (!recursion_stack.empty())
    {
        // pop next element
        const GeometryRange pair = recursion_stack.top();
        recursion_stack.pop();
        // sanity checks
        BOOST_ASSERT_MSG(pair.first->necessary, "left border must be necessary");
        BOOST_ASSERT_MSG(pair.second->necessary, "right border must be necessary");
        BOOST_ASSERT_MSG(std::distance(pair.second, end) > 0, "right border outside of geometry");
        BOOST_ASSERT_MSG(std::distance(pair.first, pair.second) >= 0,
                         "left border on the wrong side");

        int max_int_distance = 0;
        auto farthest_entry_it = pair.second;
        const CoordinatePairCalculator dist_calc(pair.first->location, pair.second->location);

        // sweep over range to find the maximum
        for (auto it = std::next(pair.first); it != pair.second; ++it)
        {
            const int distance = dist_calc(it->location);
            // found new feasible maximum?
            if (distance > max_int_distance && distance > DOUGLAS_PEUCKER_THRESHOLDS[zoom_level])
            {
                farthest_entry_it = it;
                max_int_distance = distance;
            }
        }

        // check if maximum violates a zoom level dependent threshold
        if (max_int_distance > DOUGLAS_PEUCKER_THRESHOLDS[zoom_level])
        {
            //  mark idx as necessary
            farthest_entry_it->necessary = true;
            if (1 < std::distance(pair.first, farthest_entry_it))
            {
                recursion_stack.emplace(pair.first, farthest_entry_it);
            }
            if (1 < std::distance(farthest_entry_it, pair.second))
            {
                recursion_stack.emplace(farthest_entry_it, pair.second);
            }
        }
    }
}
