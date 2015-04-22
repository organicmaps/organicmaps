/*

Copyright (c) 2014, Project OSRM, Dennis Luxen, others
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

//  This class constructs the edge-expanded routing graph

#ifndef EDGE_BASED_GRAPH_FACTORY_HPP_
#define EDGE_BASED_GRAPH_FACTORY_HPP_

#include "geometry_compressor.hpp"
#include "../typedefs.h"
#include "../data_structures/deallocating_vector.hpp"
#include "../data_structures/edge_based_node.hpp"
#include "../data_structures/original_edge_data.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/turn_instructions.hpp"
#include "../data_structures/node_based_graph.hpp"
#include "../data_structures/restriction_map.hpp"
#include "../data_structures/edge_based_node_data.hpp"

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct lua_State;

class EdgeBasedGraphFactory
{
  public:
    EdgeBasedGraphFactory() = delete;
    EdgeBasedGraphFactory(const EdgeBasedGraphFactory &) = delete;

    struct SpeedProfileProperties;

    explicit EdgeBasedGraphFactory(const std::shared_ptr<NodeBasedDynamicGraph> &node_based_graph,
                                   const std::shared_ptr<NodeBasedDynamicGraph> &node_based_graph_origin,
                                   std::unique_ptr<RestrictionMap> restricion_map,
                                   std::vector<NodeID> &barrier_node_list,
                                   std::vector<NodeID> &traffic_light_node_list,
                                   std::vector<QueryNode> &node_info_list,
                                   SpeedProfileProperties &speed_profile);

    void Run(const std::string &original_edge_data_filename,
             const std::string &geometry_filename,
             lua_State *lua_state);

    void GetEdgeBasedEdges(DeallocatingVector<EdgeBasedEdge> &edges);

    void GetEdgeBasedNodes(std::vector<EdgeBasedNode> &nodes);

    void GetEdgeBasedNodeData(osrm::NodeDataVectorT &data);

    TurnInstruction AnalyzeTurn(const NodeID u, const NodeID v, const NodeID w, const double angle) const;

    int GetTurnPenalty(double angle, lua_State *lua_state) const;

    unsigned GetNumberOfEdgeBasedNodes() const;

    struct SpeedProfileProperties
    {
        SpeedProfileProperties()
            : traffic_signal_penalty(0), u_turn_penalty(0), has_turn_penalty_function(false)
        {
        }

        int traffic_signal_penalty;
        int u_turn_penalty;
        bool has_turn_penalty_function;
    } speed_profile;

  private:
    using EdgeData = NodeBasedDynamicGraph::EdgeData;

    unsigned m_number_of_edge_based_nodes;

    std::vector<QueryNode> m_node_info_list;
    std::vector<EdgeBasedNode> m_edge_based_node_list;
    DeallocatingVector<EdgeBasedEdge> m_edge_based_edge_list;

    std::shared_ptr<NodeBasedDynamicGraph> m_node_based_graph;
    std::shared_ptr<NodeBasedDynamicGraph> m_node_based_graph_origin;
    std::unordered_set<NodeID> m_barrier_nodes;
    std::unordered_set<NodeID> m_traffic_lights;

    std::unique_ptr<RestrictionMap> m_restriction_map;
    std::vector<osrm::NodeData> m_edge_based_node_data;

    GeometryCompressor m_geometry_compressor;

    void CompressGeometry();
    void RenumberEdges();
    void GenerateEdgeBasedNodeData();
    void GenerateEdgeExpandedNodes();
    void GenerateEdgeExpandedEdges(const std::string &original_edge_data_filename,
                                   lua_State *lua_state);

    void InsertEdgeBasedNode(const NodeID u, const NodeID v, const unsigned component_id);

    void FlushVectorToStream(std::ofstream &edge_data_file,
                             std::vector<OriginalEdgeData> &original_edge_data_vector) const;

    NodeID max_id;
    std::size_t removed_node_count;

};

#endif /* EDGE_BASED_GRAPH_FACTORY_HPP_ */
