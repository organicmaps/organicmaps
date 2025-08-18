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

#ifndef EXTRACT_ROUTE_NAMES_H
#define EXTRACT_ROUTE_NAMES_H

#include <boost/assert.hpp>

#include <algorithm>
#include <string>
#include <vector>

struct RouteNames
{
    std::string shortest_path_name_1;
    std::string shortest_path_name_2;
    std::string alternative_path_name_1;
    std::string alternative_path_name_2;
};

// construct routes names
template <class DataFacadeT, class SegmentT> struct ExtractRouteNames
{
  private:
    SegmentT PickNextLongestSegment(const std::vector<SegmentT> &segment_list,
                                    const unsigned blocked_name_id) const
    {
        SegmentT result_segment;
        result_segment.length = 0;

        for (const SegmentT &segment : segment_list)
        {
            if (segment.name_id != blocked_name_id && segment.length > result_segment.length &&
                segment.name_id != 0)
            {
                result_segment = segment;
            }
        }
        return result_segment;
    }

  public:
    RouteNames operator()(std::vector<SegmentT> &shortest_path_segments,
                          std::vector<SegmentT> &alternative_path_segments,
                          const DataFacadeT *facade) const
    {
        RouteNames route_names;

        SegmentT shortest_segment_1, shortest_segment_2;
        SegmentT alternative_segment_1, alternative_segment_2;

        auto length_comperator = [](const SegmentT &a, const SegmentT &b)
        {
            return a.length > b.length;
        };
        auto name_id_comperator = [](const SegmentT &a, const SegmentT &b)
        {
            return a.name_id < b.name_id;
        };

        if (shortest_path_segments.empty())
        {
            return route_names;
        }

        // pick the longest segment for the shortest path.
        std::sort(shortest_path_segments.begin(), shortest_path_segments.end(), length_comperator);
        shortest_segment_1 = shortest_path_segments[0];
        if (!alternative_path_segments.empty())
        {
            std::sort(alternative_path_segments.begin(), alternative_path_segments.end(),
                      length_comperator);

            // also pick the longest segment for the alternative path
            alternative_segment_1 = alternative_path_segments[0];
        }

        // compute the set difference (for shortest path) depending on names between shortest and
        // alternative
        std::vector<SegmentT> shortest_path_set_difference(shortest_path_segments.size());
        std::sort(shortest_path_segments.begin(), shortest_path_segments.end(), name_id_comperator);
        std::sort(alternative_path_segments.begin(), alternative_path_segments.end(),
                  name_id_comperator);
        std::set_difference(shortest_path_segments.begin(), shortest_path_segments.end(),
                            alternative_path_segments.begin(), alternative_path_segments.end(),
                            shortest_path_set_difference.begin(), name_id_comperator);

        std::sort(shortest_path_set_difference.begin(), shortest_path_set_difference.end(),
                  length_comperator);
        shortest_segment_2 =
            PickNextLongestSegment(shortest_path_set_difference, shortest_segment_1.name_id);

        // compute the set difference (for alternative path) depending on names between shortest and
        // alternative
        // vectors are still sorted, no need to do again
        BOOST_ASSERT(std::is_sorted(shortest_path_segments.begin(), shortest_path_segments.end(),
                                    name_id_comperator));
        BOOST_ASSERT(std::is_sorted(alternative_path_segments.begin(),
                                    alternative_path_segments.end(), name_id_comperator));

        std::vector<SegmentT> alternative_path_set_difference(alternative_path_segments.size());
        std::set_difference(alternative_path_segments.begin(), alternative_path_segments.end(),
                            shortest_path_segments.begin(), shortest_path_segments.end(),
                            alternative_path_set_difference.begin(), name_id_comperator);

        std::sort(alternative_path_set_difference.begin(), alternative_path_set_difference.end(),
                  length_comperator);

        if (!alternative_path_segments.empty())
        {
            alternative_segment_2 = PickNextLongestSegment(alternative_path_set_difference,
                                                           alternative_segment_1.name_id);
        }

        // move the segments into the order in which they occur.
        if (shortest_segment_1.position > shortest_segment_2.position)
        {
            std::swap(shortest_segment_1, shortest_segment_2);
        }
        if (alternative_segment_1.position > alternative_segment_2.position)
        {
            std::swap(alternative_segment_1, alternative_segment_2);
        }

        // fetching names for the selected segments
        route_names.shortest_path_name_1 = facade->get_name_for_id(shortest_segment_1.name_id);
        route_names.shortest_path_name_2 = facade->get_name_for_id(shortest_segment_2.name_id);

        route_names.alternative_path_name_1 =
            facade->get_name_for_id(alternative_segment_1.name_id);
        route_names.alternative_path_name_2 =
            facade->get_name_for_id(alternative_segment_2.name_id);

        return route_names;
    }
};

#endif // EXTRACT_ROUTE_NAMES_H
