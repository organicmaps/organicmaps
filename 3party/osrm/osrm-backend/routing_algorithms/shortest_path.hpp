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

#ifndef SHORTEST_PATH_HPP
#define SHORTEST_PATH_HPP

#include <boost/assert.hpp>

#include "routing_base.hpp"
#include "../data_structures/search_engine_data.hpp"
#include "../util/integer_range.hpp"
#include "../typedefs.h"

template <class DataFacadeT>
class ShortestPathRouting final
    : public BasicRoutingInterface<DataFacadeT, ShortestPathRouting<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, ShortestPathRouting<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

  public:
    ShortestPathRouting(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    ~ShortestPathRouting() {}

    void operator()(const std::vector<PhantomNodes> &phantom_nodes_vector,
                    const std::vector<bool> &uturn_indicators,
                    InternalRouteResult &raw_route_data) const
    {
        int distance1 = 0;
        int distance2 = 0;
        bool search_from_1st_node = true;
        bool search_from_2nd_node = true;
        NodeID middle1 = SPECIAL_NODEID;
        NodeID middle2 = SPECIAL_NODEID;
        std::vector<std::vector<NodeID>> packed_legs1(phantom_nodes_vector.size());
        std::vector<std::vector<NodeID>> packed_legs2(phantom_nodes_vector.size());

        engine_working_data.InitializeOrClearFirstThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        engine_working_data.InitializeOrClearSecondThreadLocalStorage(
            super::facade->GetNumberOfNodes());
        engine_working_data.InitializeOrClearThirdThreadLocalStorage(
            super::facade->GetNumberOfNodes());

        QueryHeap &forward_heap1 = *(engine_working_data.forward_heap_1);
        QueryHeap &reverse_heap1 = *(engine_working_data.reverse_heap_1);
        QueryHeap &forward_heap2 = *(engine_working_data.forward_heap_2);
        QueryHeap &reverse_heap2 = *(engine_working_data.reverse_heap_2);

        std::size_t current_leg = 0;
        // Get distance to next pair of target nodes.
        for (const PhantomNodes &phantom_node_pair : phantom_nodes_vector)
        {
            forward_heap1.Clear();
            forward_heap2.Clear();
            reverse_heap1.Clear();
            reverse_heap2.Clear();
            int local_upper_bound1 = INVALID_EDGE_WEIGHT;
            int local_upper_bound2 = INVALID_EDGE_WEIGHT;

            middle1 = SPECIAL_NODEID;
            middle2 = SPECIAL_NODEID;

            const bool allow_u_turn = current_leg > 0 && uturn_indicators.size() > current_leg &&
                                      uturn_indicators[current_leg - 1];
            const EdgeWeight min_edge_offset =
                std::min(phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                         phantom_node_pair.source_phantom.GetReverseWeightPlusOffset());

            // insert new starting nodes into forward heap, adjusted by previous distances.
            if ((allow_u_turn || search_from_1st_node) &&
                phantom_node_pair.source_phantom.forward_node_id != SPECIAL_NODEID)
            {
                forward_heap1.Insert(
                    phantom_node_pair.source_phantom.forward_node_id,
                    -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                    phantom_node_pair.source_phantom.forward_node_id);
                // SimpleLogger().Write(logDEBUG) << "fwd-a2 insert: " <<
                // phantom_node_pair.source_phantom.forward_node_id << ", w: " << -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset();
                forward_heap2.Insert(
                    phantom_node_pair.source_phantom.forward_node_id,
                    -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset(),
                    phantom_node_pair.source_phantom.forward_node_id);
                // SimpleLogger().Write(logDEBUG) << "fwd-b2 insert: " <<
                // phantom_node_pair.source_phantom.forward_node_id << ", w: " << -phantom_node_pair.source_phantom.GetForwardWeightPlusOffset();
            }
            if ((allow_u_turn || search_from_2nd_node) &&
                phantom_node_pair.source_phantom.reverse_node_id != SPECIAL_NODEID)
            {
                forward_heap1.Insert(
                    phantom_node_pair.source_phantom.reverse_node_id,
                    -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset(),
                    phantom_node_pair.source_phantom.reverse_node_id);
                // SimpleLogger().Write(logDEBUG) << "fwd-a2 insert: " <<
                // phantom_node_pair.source_phantom.reverse_node_id << ", w: " << -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset();
                forward_heap2.Insert(
                    phantom_node_pair.source_phantom.reverse_node_id,
                    -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset(),
                    phantom_node_pair.source_phantom.reverse_node_id);
                // SimpleLogger().Write(logDEBUG) << "fwd-b2 insert: " <<
                // phantom_node_pair.source_phantom.reverse_node_id << ", w: " << -phantom_node_pair.source_phantom.GetReverseWeightPlusOffset();
            }

            // insert new backward nodes into backward heap, unadjusted.
            if (phantom_node_pair.target_phantom.forward_node_id != SPECIAL_NODEID)
            {
                reverse_heap1.Insert(phantom_node_pair.target_phantom.forward_node_id,
                                     phantom_node_pair.target_phantom.GetForwardWeightPlusOffset(),
                                     phantom_node_pair.target_phantom.forward_node_id);
                // SimpleLogger().Write(logDEBUG) << "rev-a insert: " <<
                // phantom_node_pair.target_phantom.forward_node_id << ", w: " << phantom_node_pair.target_phantom.GetForwardWeightPlusOffset();
            }

            if (phantom_node_pair.target_phantom.reverse_node_id != SPECIAL_NODEID)
            {
                reverse_heap2.Insert(phantom_node_pair.target_phantom.reverse_node_id,
                                     phantom_node_pair.target_phantom.GetReverseWeightPlusOffset(),
                                     phantom_node_pair.target_phantom.reverse_node_id);
                // SimpleLogger().Write(logDEBUG) << "rev-a insert: " <<
                // phantom_node_pair.target_phantom.reverse_node_id << ", w: " << phantom_node_pair.target_phantom.GetReverseWeightPlusOffset();
            }

            // run two-Target Dijkstra routing step.
            while (0 < (forward_heap1.Size() + reverse_heap1.Size()))
            {
                if (!forward_heap1.Empty())
                {
                    super::RoutingStep(forward_heap1, reverse_heap1, &middle1, &local_upper_bound1,
                                       min_edge_offset, true);
                }
                if (!reverse_heap1.Empty())
                {
                    super::RoutingStep(reverse_heap1, forward_heap1, &middle1, &local_upper_bound1,
                                       min_edge_offset, false);
                }
            }

            if (!reverse_heap2.Empty())
            {
                while (0 < (forward_heap2.Size() + reverse_heap2.Size()))
                {
                    if (!forward_heap2.Empty())
                    {
                        super::RoutingStep(forward_heap2, reverse_heap2, &middle2,
                                           &local_upper_bound2, min_edge_offset, true);
                    }
                    if (!reverse_heap2.Empty())
                    {
                        super::RoutingStep(reverse_heap2, forward_heap2, &middle2,
                                           &local_upper_bound2, min_edge_offset, false);
                    }
                }
            }

            // No path found for both target nodes?
            if ((INVALID_EDGE_WEIGHT == local_upper_bound1) &&
                (INVALID_EDGE_WEIGHT == local_upper_bound2))
            {
                raw_route_data.shortest_path_length = INVALID_EDGE_WEIGHT;
                raw_route_data.alternative_path_length = INVALID_EDGE_WEIGHT;
                return;
            }

            search_from_1st_node = true;
            search_from_2nd_node = true;
            if (SPECIAL_NODEID == middle1)
            {
                search_from_1st_node = false;
            }
            if (SPECIAL_NODEID == middle2)
            {
                search_from_2nd_node = false;
            }

            // Was at most one of the two paths not found?
            BOOST_ASSERT_MSG((INVALID_EDGE_WEIGHT != distance1 || INVALID_EDGE_WEIGHT != distance2),
                             "no path found");

            // Unpack paths if they exist
            std::vector<NodeID> temporary_packed_leg1;
            std::vector<NodeID> temporary_packed_leg2;

            BOOST_ASSERT(current_leg < packed_legs1.size());
            BOOST_ASSERT(current_leg < packed_legs2.size());

            if (INVALID_EDGE_WEIGHT != local_upper_bound1)
            {
                super::RetrievePackedPathFromHeap(forward_heap1, reverse_heap1, middle1,
                                                  temporary_packed_leg1);
            }

            if (INVALID_EDGE_WEIGHT != local_upper_bound2)
            {
                super::RetrievePackedPathFromHeap(forward_heap2, reverse_heap2, middle2,
                                                  temporary_packed_leg2);
            }

            // if one of the paths was not found, replace it with the other one.
            if ((allow_u_turn && local_upper_bound1 > local_upper_bound2) ||
                temporary_packed_leg1.empty())
            {
                temporary_packed_leg1.clear();
                temporary_packed_leg1.insert(temporary_packed_leg1.end(),
                                             temporary_packed_leg2.begin(),
                                             temporary_packed_leg2.end());
                local_upper_bound1 = local_upper_bound2;
            }
            if ((allow_u_turn && local_upper_bound2 > local_upper_bound1) ||
                temporary_packed_leg2.empty())
            {
                temporary_packed_leg2.clear();
                temporary_packed_leg2.insert(temporary_packed_leg2.end(),
                                             temporary_packed_leg1.begin(),
                                             temporary_packed_leg1.end());
                local_upper_bound2 = local_upper_bound1;
            }

            BOOST_ASSERT_MSG(!temporary_packed_leg1.empty() || !temporary_packed_leg2.empty(),
                             "tempory packed paths empty");

            BOOST_ASSERT((0 == current_leg) || !packed_legs1[current_leg - 1].empty());
            BOOST_ASSERT((0 == current_leg) || !packed_legs2[current_leg - 1].empty());

            if (!allow_u_turn && 0 < current_leg)
            {
                const NodeID end_id_of_segment1 = packed_legs1[current_leg - 1].back();
                const NodeID end_id_of_segment2 = packed_legs2[current_leg - 1].back();
                BOOST_ASSERT(!temporary_packed_leg1.empty());
                const NodeID start_id_of_leg1 = temporary_packed_leg1.front();
                const NodeID start_id_of_leg2 = temporary_packed_leg2.front();
                if ((end_id_of_segment1 != start_id_of_leg1) &&
                    (end_id_of_segment2 != start_id_of_leg2))
                {
                    std::swap(temporary_packed_leg1, temporary_packed_leg2);
                    std::swap(local_upper_bound1, local_upper_bound2);
                }

                // remove the shorter path if both legs end at the same segment
                if (start_id_of_leg1 == start_id_of_leg2)
                {
                    const NodeID last_id_of_packed_legs1 = packed_legs1[current_leg - 1].back();
                    const NodeID last_id_of_packed_legs2 = packed_legs2[current_leg - 1].back();
                    if (start_id_of_leg1 != last_id_of_packed_legs1)
                    {
                        packed_legs1 = packed_legs2;
                        BOOST_ASSERT(start_id_of_leg1 == temporary_packed_leg1.front());
                    }
                    else if (start_id_of_leg2 != last_id_of_packed_legs2)
                    {
                        packed_legs2 = packed_legs1;
                        BOOST_ASSERT(start_id_of_leg2 == temporary_packed_leg2.front());
                    }
                }
            }
            BOOST_ASSERT(packed_legs1.size() == packed_legs2.size());

            packed_legs1[current_leg].insert(packed_legs1[current_leg].end(),
                                             temporary_packed_leg1.begin(),
                                             temporary_packed_leg1.end());
            BOOST_ASSERT(packed_legs1[current_leg].size() == temporary_packed_leg1.size());
            packed_legs2[current_leg].insert(packed_legs2[current_leg].end(),
                                             temporary_packed_leg2.begin(),
                                             temporary_packed_leg2.end());
            BOOST_ASSERT(packed_legs2[current_leg].size() == temporary_packed_leg2.size());

            if (!allow_u_turn &&
                (packed_legs1[current_leg].back() == packed_legs2[current_leg].back()) &&
                phantom_node_pair.target_phantom.is_bidirected())
            {
                const NodeID last_node_id = packed_legs2[current_leg].back();
                search_from_1st_node &=
                    !(last_node_id == phantom_node_pair.target_phantom.reverse_node_id);
                search_from_2nd_node &=
                    !(last_node_id == phantom_node_pair.target_phantom.forward_node_id);
                BOOST_ASSERT(search_from_1st_node != search_from_2nd_node);
            }

            distance1 += local_upper_bound1;
            distance2 += local_upper_bound2;
            ++current_leg;
        }

        if (distance1 > distance2)
        {
            std::swap(packed_legs1, packed_legs2);
        }
        raw_route_data.unpacked_path_segments.resize(packed_legs1.size());

        for (const std::size_t index : osrm::irange<std::size_t>(0, packed_legs1.size()))
        {
            BOOST_ASSERT(!phantom_nodes_vector.empty());
            BOOST_ASSERT(packed_legs1.size() == raw_route_data.unpacked_path_segments.size());

            PhantomNodes unpack_phantom_node_pair = phantom_nodes_vector[index];
            super::UnpackPath(
                // -- packed input
                packed_legs1[index],
                // -- start and end of (sub-)route
                unpack_phantom_node_pair,
                // -- unpacked output
                raw_route_data.unpacked_path_segments[index]);

            raw_route_data.source_traversed_in_reverse.push_back(
                (packed_legs1[index].front() !=
                 phantom_nodes_vector[index].source_phantom.forward_node_id));
            raw_route_data.target_traversed_in_reverse.push_back(
                (packed_legs1[index].back() !=
                 phantom_nodes_vector[index].target_phantom.forward_node_id));
        }
        raw_route_data.shortest_path_length = std::min(distance1, distance2);
    }
};

#endif /* SHORTEST_PATH_HPP */
