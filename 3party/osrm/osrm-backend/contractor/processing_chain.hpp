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

#ifndef PROCESSING_CHAIN_HPP
#define PROCESSING_CHAIN_HPP

#include "edge_based_graph_factory.hpp"
#include "../data_structures/query_edge.hpp"
#include "../data_structures/static_graph.hpp"

class FingerPrint;
struct EdgeBasedNode;
struct lua_State;

#include <boost/filesystem.hpp>

#include <vector>

/**
    \brief class of 'prepare' utility.
 */
class Prepare
{
  public:
    using EdgeData = QueryEdge::EdgeData;
    using InputEdge = DynamicGraph<EdgeData>::InputEdge;
    using StaticEdge = StaticGraph<EdgeData>::InputEdge;

    explicit Prepare();
    Prepare(const Prepare &) = delete;
    ~Prepare();

    int Process(int argc, char *argv[]);

  protected:
    bool ParseArguments(int argc, char *argv[]);
    void CheckRestrictionsFile(FingerPrint &fingerprint_orig);
    bool SetupScriptingEnvironment(lua_State *myLuaState,
                                   EdgeBasedGraphFactory::SpeedProfileProperties &speed_profile);
    std::size_t BuildEdgeExpandedGraph(lua_State *myLuaState,
                                       NodeID nodeBasedNodeNumber,
                                       std::vector<EdgeBasedNode> &nodeBasedEdgeList,
                                       DeallocatingVector<EdgeBasedEdge> &edgeBasedEdgeList,
                                       EdgeBasedGraphFactory::SpeedProfileProperties &speed_profile);
    void WriteNodeMapping();
    void BuildRTree(std::vector<EdgeBasedNode> &node_based_edge_list);

  private:
    std::vector<QueryNode> internal_to_external_node_map;
    std::vector<TurnRestriction> restriction_list;
    std::vector<NodeID> barrier_node_list;
    std::vector<NodeID> traffic_light_list;
    std::vector<ImportEdge> edge_list;

    unsigned requested_num_threads;
    boost::filesystem::path config_file_path;
    boost::filesystem::path input_path;
    boost::filesystem::path restrictions_path;
    boost::filesystem::path preinfo_path;
    boost::filesystem::path profile_path;

    std::string node_filename;
    std::string edge_out;
    std::string info_out;
    std::string geometry_filename;
    std::string graph_out;
    std::string rtree_nodes_path;
    std::string rtree_leafs_path;
    std::string node_data_filename;
};

#endif // PROCESSING_CHAIN_HPP
