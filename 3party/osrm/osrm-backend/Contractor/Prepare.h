#ifndef PREPARE_H
#define PREPARE_H

#include "EdgeBasedGraphFactory.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/StaticGraph.h"
#include "../Util/GraphLoader.h"

#include <boost/filesystem.hpp>

#include <luabind/luabind.hpp>

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
    std::vector<NodeInfo> internal_to_external_node_map;
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
};

#endif // PREPARE_H
