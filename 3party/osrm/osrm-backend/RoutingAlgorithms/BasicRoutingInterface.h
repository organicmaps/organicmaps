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

#ifndef BASIC_ROUTING_INTERFACE_H
#define BASIC_ROUTING_INTERFACE_H

#include "../DataStructures/RawRouteData.h"
#include "../DataStructures/SearchEngineData.h"
#include "../DataStructures/TurnInstructions.h"
// #include "../Util/simple_logger.hpp.h"

#include <boost/assert.hpp>

#include <stack>

SearchEngineData::SearchEngineHeapPtr SearchEngineData::forwardHeap;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::backwardHeap;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forwardHeap2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::backwardHeap2;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::forwardHeap3;
SearchEngineData::SearchEngineHeapPtr SearchEngineData::backwardHeap3;

template <class DataFacadeT> class BasicRoutingInterface
{
  private:
    typedef typename DataFacadeT::EdgeData EdgeData;

  protected:
    DataFacadeT *facade;

  public:
    BasicRoutingInterface() = delete;
    BasicRoutingInterface(const BasicRoutingInterface &) = delete;
    explicit BasicRoutingInterface(DataFacadeT *facade) : facade(facade) {}
    virtual ~BasicRoutingInterface() {};

    inline void RoutingStep(SearchEngineData::QueryHeap &forward_heap,
                            SearchEngineData::QueryHeap &reverse_heap,
                            NodeID *middle_node_id,
                            int *upper_bound,
                            const int min_edge_offset,
                            const bool forward_direction) const
    {
        const NodeID node = forward_heap.DeleteMin();
        const int distance = forward_heap.GetKey(node);

        // const NodeID parentnode = forward_heap.GetData(node).parent;
        // SimpleLogger().Write() << (forward_direction ? "[fwd] " : "[rev] ") << "settled edge (" << parentnode << "," << node << "), dist: " << distance;

        if (reverse_heap.WasInserted(node))
        {
            const int new_distance = reverse_heap.GetKey(node) + distance;
            if (new_distance < *upper_bound)
            {
                if (new_distance >= 0)
                {
                    *middle_node_id = node;
                    *upper_bound = new_distance;
                //     SimpleLogger().Write() << "accepted middle node " << node << " at distance " << new_distance;
                // } else {
                //     SimpleLogger().Write() << "discared middle node " << node << " at distance " << new_distance;
                }
            }
        }

        if (distance + min_edge_offset > *upper_bound)
        {
            // SimpleLogger().Write() << "min_edge_offset: " << min_edge_offset;
            forward_heap.DeleteAll();
            return;
        }

        // Stalling
        for (const auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const EdgeData &data = facade->GetEdgeData(edge);
            const bool reverse_flag = ((!forward_direction) ? data.forward : data.backward);
            if (reverse_flag)
            {
                const NodeID to = facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");

                if (forward_heap.WasInserted(to))
                {
                    if (forward_heap.GetKey(to) + edge_weight < distance)
                    {
                        return;
                    }
                }
            }
        }

        for (const auto edge : facade->GetAdjacentEdgeRange(node))
        {
            const EdgeData &data = facade->GetEdgeData(edge);
            bool forward_directionFlag = (forward_direction ? data.forward : data.backward);
            if (forward_directionFlag)
            {

                const NodeID to = facade->GetTarget(edge);
                const int edge_weight = data.distance;

                BOOST_ASSERT_MSG(edge_weight > 0, "edge_weight invalid");
                const int to_distance = distance + edge_weight;

                // New Node discovered -> Add to Heap + Node Info Storage
                if (!forward_heap.WasInserted(to))
                {
                    forward_heap.Insert(to, to_distance, node);
                }
                // Found a shorter Path -> Update distance
                else if (to_distance < forward_heap.GetKey(to))
                {
                    // new parent
                    forward_heap.GetData(to).parent = node;
                    forward_heap.DecreaseKey(to, to_distance);
                }
            }
        }
    }

    inline void UnpackPath(const std::vector<NodeID> &packed_path,
                           const PhantomNodes &phantom_node_pair,
                           std::vector<PathData> &unpacked_path) const
    {
        const bool start_traversed_in_reverse =
            (packed_path.front() != phantom_node_pair.source_phantom.forward_node_id);
        const bool target_traversed_in_reverse =
            (packed_path.back() != phantom_node_pair.target_phantom.forward_node_id);

        const unsigned packed_path_size = static_cast<unsigned>(packed_path.size());
        std::stack<std::pair<NodeID, NodeID>> recursion_stack;

        // We have to push the path in reverse order onto the stack because it's LIFO.
        for (unsigned i = packed_path_size - 1; i > 0; --i)
        {
            recursion_stack.emplace(packed_path[i - 1], packed_path[i]);
        }

        std::pair<NodeID, NodeID> edge;
        while (!recursion_stack.empty())
        {
            /*
            Graphical representation of variables:

            edge.first         edge.second
                *------------------>*
                       edge_id
            */
            edge = recursion_stack.top();
            recursion_stack.pop();

            // facade->FindEdge does not suffice here in case of shortcuts.
            // The above explanation unclear? Think!
            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            int edge_weight = std::numeric_limits<EdgeWeight>::max();
            for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.first))
            {
                const int weight = facade->GetEdgeData(edge_id).distance;
                if ((facade->GetTarget(edge_id) == edge.second) && (weight < edge_weight) &&
                    facade->GetEdgeData(edge_id).forward)
                {
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            /*
                Graphical representation of variables:

                edge.first         edge.second
                    *<------------------*
                           edge_id
            */
            if (SPECIAL_EDGEID == smaller_edge_id)
            {
                for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.second))
                {
                    const int weight = facade->GetEdgeData(edge_id).distance;
                    if ((facade->GetTarget(edge_id) == edge.first) && (weight < edge_weight) &&
                        facade->GetEdgeData(edge_id).backward)
                    {
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != INVALID_EDGE_WEIGHT, "edge id invalid");

            const EdgeData &ed = facade->GetEdgeData(smaller_edge_id);
            if (ed.shortcut)
            { // unpack
                const NodeID middle_node_id = ed.id;
                // again, we need to this in reversed order
                recursion_stack.emplace(middle_node_id, edge.second);
                recursion_stack.emplace(edge.first, middle_node_id);
            }
            else
            {
                BOOST_ASSERT_MSG(!ed.shortcut, "original edge flagged as shortcut");
                unsigned name_index = facade->GetNameIndexFromEdgeID(ed.id);
                const TurnInstruction turn_instruction = facade->GetTurnInstructionForEdgeID(ed.id);
                const TravelMode travel_mode = facade->GetTravelModeForEdgeID(ed.id);


                if (!facade->EdgeIsCompressed(ed.id))
                {
                    BOOST_ASSERT(!facade->EdgeIsCompressed(ed.id));
                    unpacked_path.emplace_back(facade->GetGeometryIndexForEdgeID(ed.id),
                                               name_index,
                                               turn_instruction,
                                               ed.distance,
                                               travel_mode);
                }
                else
                {
                    std::vector<unsigned> id_vector;
                    facade->GetUncompressedGeometry(facade->GetGeometryIndexForEdgeID(ed.id),
                                                    id_vector);

                    const std::size_t start_index =
                        (unpacked_path.empty()
                             ? ((start_traversed_in_reverse)
                                    ? id_vector.size() -
                                          phantom_node_pair.source_phantom.fwd_segment_position - 1
                                    : phantom_node_pair.source_phantom.fwd_segment_position)
                             : 0);
                    const std::size_t end_index = id_vector.size();

                    BOOST_ASSERT(start_index >= 0);
                    BOOST_ASSERT(start_index <= end_index);
                    for (std::size_t i = start_index; i < end_index; ++i)
                    {
                        unpacked_path.emplace_back(id_vector[i], name_index, TurnInstruction::NoTurn, 0, travel_mode);
                    }
                    unpacked_path.back().turn_instruction = turn_instruction;
                    unpacked_path.back().segment_duration = ed.distance;
                }
            }
        }
        if (SPECIAL_EDGEID != phantom_node_pair.target_phantom.packed_geometry_id)
        {
            std::vector<unsigned> id_vector;
            facade->GetUncompressedGeometry(phantom_node_pair.target_phantom.packed_geometry_id,
                                            id_vector);
            const bool is_local_path = (phantom_node_pair.source_phantom.packed_geometry_id ==
                                        phantom_node_pair.target_phantom.packed_geometry_id) &&
                                       unpacked_path.empty();

            std::size_t start_index = 0;
            if (is_local_path)
            {
                start_index = phantom_node_pair.source_phantom.fwd_segment_position;
                if (target_traversed_in_reverse)
                {
                    start_index =
                        id_vector.size() - phantom_node_pair.source_phantom.fwd_segment_position;
                }
            }

            std::size_t end_index = phantom_node_pair.target_phantom.fwd_segment_position;
            if (target_traversed_in_reverse)
            {
                std::reverse(id_vector.begin(), id_vector.end());
                end_index =
                    id_vector.size() - phantom_node_pair.target_phantom.fwd_segment_position;
            }

            if (start_index > end_index)
            {
                start_index = std::min(start_index, id_vector.size()-1);
            }

            for (std::size_t i = start_index; i != end_index; (start_index < end_index ? ++i : --i))
            {
                BOOST_ASSERT(i < id_vector.size());
                BOOST_ASSERT(phantom_node_pair.target_phantom.forward_travel_mode>0 );
                unpacked_path.emplace_back(PathData{id_vector[i],
                                                    phantom_node_pair.target_phantom.name_id,
                                                    TurnInstruction::NoTurn,
                                                    0,
                                                    phantom_node_pair.target_phantom.forward_travel_mode});
            }
        }

        // there is no equivalent to a node-based node in an edge-expanded graph.
        // two equivalent routes may start (or end) at different node-based edges
        // as they are added with the offset how much "distance" on the edge
        // has already been traversed. Depending on offset one needs to remove
        // the last node.
        if (unpacked_path.size() > 1)
        {
            const std::size_t last_index = unpacked_path.size() - 1;
            const std::size_t second_to_last_index = last_index - 1;

            // looks like a trivially true check but tests for underflow
            BOOST_ASSERT(last_index > second_to_last_index);

            if (unpacked_path[last_index].node == unpacked_path[second_to_last_index].node)
            {
                unpacked_path.pop_back();
            }
            BOOST_ASSERT(!unpacked_path.empty());
        }
    }

    inline void UnpackEdge(const NodeID s, const NodeID t, std::vector<NodeID> &unpacked_path) const
    {
        std::stack<std::pair<NodeID, NodeID>> recursion_stack;
        recursion_stack.emplace(s, t);

        std::pair<NodeID, NodeID> edge;
        while (!recursion_stack.empty())
        {
            edge = recursion_stack.top();
            recursion_stack.pop();

            EdgeID smaller_edge_id = SPECIAL_EDGEID;
            int edge_weight = std::numeric_limits<EdgeWeight>::max();
            for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.first))
            {
                const int weight = facade->GetEdgeData(edge_id).distance;
                if ((facade->GetTarget(edge_id) == edge.second) && (weight < edge_weight) &&
                    facade->GetEdgeData(edge_id).forward)
                {
                    smaller_edge_id = edge_id;
                    edge_weight = weight;
                }
            }

            if (SPECIAL_EDGEID == smaller_edge_id)
            {
                for (const auto edge_id : facade->GetAdjacentEdgeRange(edge.second))
                {
                    const int weight = facade->GetEdgeData(edge_id).distance;
                    if ((facade->GetTarget(edge_id) == edge.first) && (weight < edge_weight) &&
                        facade->GetEdgeData(edge_id).backward)
                    {
                        smaller_edge_id = edge_id;
                        edge_weight = weight;
                    }
                }
            }
            BOOST_ASSERT_MSG(edge_weight != std::numeric_limits<EdgeWeight>::max(), "edge weight invalid");

            const EdgeData &ed = facade->GetEdgeData(smaller_edge_id);
            if (ed.shortcut)
            { // unpack
                const NodeID middle_node_id = ed.id;
                // again, we need to this in reversed order
                recursion_stack.emplace(middle_node_id, edge.second);
                recursion_stack.emplace(edge.first, middle_node_id);
            }
            else
            {
                BOOST_ASSERT_MSG(!ed.shortcut, "edge must be shortcut");
                unpacked_path.emplace_back(edge.first);
            }
        }
        unpacked_path.emplace_back(t);
    }

    inline void RetrievePackedPathFromHeap(const SearchEngineData::QueryHeap &forward_heap,
                                           const SearchEngineData::QueryHeap &reverse_heap,
                                           const NodeID middle_node_id,
                                           std::vector<NodeID> &packed_path) const
    {
        RetrievePackedPathFromSingleHeap(forward_heap, middle_node_id, packed_path);
        std::reverse(packed_path.begin(), packed_path.end());
        packed_path.emplace_back(middle_node_id);
        RetrievePackedPathFromSingleHeap(reverse_heap, middle_node_id, packed_path);
    }

    inline void RetrievePackedPathFromSingleHeap(const SearchEngineData::QueryHeap &search_heap,
                                                 const NodeID middle_node_id,
                                                 std::vector<NodeID> &packed_path) const
    {
        NodeID current_node_id = middle_node_id;
        while (current_node_id != search_heap.GetData(current_node_id).parent)
        {
            current_node_id = search_heap.GetData(current_node_id).parent;
            packed_path.emplace_back(current_node_id);
        }
    }
};

#endif // BASIC_ROUTING_INTERFACE_H
