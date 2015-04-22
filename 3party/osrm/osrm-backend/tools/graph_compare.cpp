#include "../data_structures/dynamic_graph.hpp"
#include "../data_structures/import_edge.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/restriction.hpp"
#include "../data_structures/static_graph.hpp"
#include "../util/fingerprint.hpp"
#include "../util/graph_loader.hpp"
#include "../util/integer_range.hpp"
#include "../util/make_unique.hpp"
#include "../util/osrm_exception.hpp"
#include "../util/simple_logger.hpp"

#include "../typedefs.h"

#include <algorithm>
#include <fstream>

struct TarjanEdgeData
{
    TarjanEdgeData() : distance(INVALID_EDGE_WEIGHT), name_id(INVALID_NAMEID) {}
    TarjanEdgeData(unsigned distance, unsigned name_id) : distance(distance), name_id(name_id) {}
    unsigned distance;
    unsigned name_id;
};

using StaticTestGraph = StaticGraph<TarjanEdgeData>;
using DynamicTestGraph = StaticGraph<TarjanEdgeData>;
using StaticEdge = StaticTestGraph::InputEdge;
using DynamicEdge = DynamicTestGraph::InputEdge;

int main(int argc, char *argv[])
{
    std::vector<QueryNode> coordinate_list;
    std::vector<TurnRestriction> restriction_list;
    std::vector<NodeID> bollard_node_list;
    std::vector<NodeID> traffic_lights_list;

    LogPolicy::GetInstance().Unmute();
    try
    {
        // enable logging
        if (argc < 3)
        {
            SimpleLogger().Write(logWARNING) << "usage:\n" << argv[0]
                                             << " <osrm> <osrm.restrictions>";
            return -1;
        }

        SimpleLogger().Write() << "Using restrictions from file: " << argv[2];
        std::ifstream restriction_ifstream(argv[2], std::ios::binary);
        const FingerPrint fingerprint_orig;
        FingerPrint fingerprint_loaded;
        restriction_ifstream.read(reinterpret_cast<char *>(&fingerprint_loaded),
                                  sizeof(FingerPrint));

        // check fingerprint and warn if necessary
        if (!fingerprint_loaded.TestGraphUtil(fingerprint_orig))
        {
            SimpleLogger().Write(logWARNING) << argv[2] << " was prepared with a different build. "
                                                           "Reprocess to get rid of this warning.";
        }

        if (!restriction_ifstream.good())
        {
            throw osrm::exception("Could not access <osrm-restrictions> files");
        }
        uint32_t usable_restrictions = 0;
        restriction_ifstream.read(reinterpret_cast<char *>(&usable_restrictions), sizeof(uint32_t));
        restriction_list.resize(usable_restrictions);

        // load restrictions
        if (usable_restrictions > 0)
        {
            restriction_ifstream.read(reinterpret_cast<char *>(&restriction_list[0]),
                                      usable_restrictions * sizeof(TurnRestriction));
        }
        restriction_ifstream.close();

        std::ifstream input_stream(argv[1], std::ifstream::in | std::ifstream::binary);
        if (!input_stream.is_open())
        {
            throw osrm::exception("Cannot open osrm file");
        }

        // load graph data
        std::vector<ImportEdge> edge_list;
        const NodeID number_of_nodes =
            readBinaryOSRMGraphFromStream(input_stream, edge_list, bollard_node_list,
                                          traffic_lights_list, &coordinate_list, restriction_list);
        input_stream.close();

        BOOST_ASSERT_MSG(restriction_list.size() == usable_restrictions,
                         "size of restriction_list changed");

        SimpleLogger().Write() << restriction_list.size() << " restrictions, "
                               << bollard_node_list.size() << " bollard nodes, "
                               << traffic_lights_list.size() << " traffic lights";

        traffic_lights_list.clear();
        traffic_lights_list.shrink_to_fit();

        // Building an node-based graph
        std::vector<StaticEdge> static_graph_edge_list;
        std::vector<DynamicEdge> dynamic_graph_edge_list;
        for (const auto &input_edge : edge_list)
        {
            if (input_edge.source == input_edge.target)
            {
                continue;
            }

            if (input_edge.forward)
            {
                static_graph_edge_list.emplace_back(input_edge.source, input_edge.target,
                                                    (std::max)(input_edge.weight, 1),
                                                    input_edge.name_id);
                dynamic_graph_edge_list.emplace_back(input_edge.source, input_edge.target,
                                                     (std::max)(input_edge.weight, 1),
                                                     input_edge.name_id);
            }
            if (input_edge.backward)
            {
                dynamic_graph_edge_list.emplace_back(input_edge.target, input_edge.source,
                                                     (std::max)(input_edge.weight, 1),
                                                     input_edge.name_id);
                static_graph_edge_list.emplace_back(input_edge.target, input_edge.source,
                                                    (std::max)(input_edge.weight, 1),
                                                    input_edge.name_id);
            }
        }
        edge_list.clear();
        edge_list.shrink_to_fit();
        BOOST_ASSERT_MSG(0 == edge_list.size() && 0 == edge_list.capacity(),
                         "input edge vector not properly deallocated");

        tbb::parallel_sort(static_graph_edge_list.begin(), static_graph_edge_list.end());
        tbb::parallel_sort(dynamic_graph_edge_list.begin(), dynamic_graph_edge_list.end());

        auto static_graph =
            osrm::make_unique<StaticTestGraph>(number_of_nodes, static_graph_edge_list);
        auto dynamic_graph =
            osrm::make_unique<DynamicTestGraph>(number_of_nodes, dynamic_graph_edge_list);

        SimpleLogger().Write() << "Starting static/dynamic graph comparison";

        BOOST_ASSERT(static_graph->GetNumberOfNodes() == dynamic_graph->GetNumberOfNodes());
        BOOST_ASSERT(static_graph->GetNumberOfEdges() == dynamic_graph->GetNumberOfEdges());
        for (const auto node : osrm::irange(0u, static_graph->GetNumberOfNodes()))
        {
            const auto static_range = static_graph->GetAdjacentEdgeRange(node);
            const auto dynamic_range = dynamic_graph->GetAdjacentEdgeRange(node);
            SimpleLogger().Write() << "checking node " << node << "/"
                                   << static_graph->GetNumberOfNodes();

            BOOST_ASSERT(static_range.size() == dynamic_range.size());
            const auto static_begin = static_graph->BeginEdges(node);
            const auto dynamic_begin = dynamic_graph->BeginEdges(node);

            // check raw interface
            for (const auto i : osrm::irange(0u, static_range.size()))
            {
                const auto static_target = static_graph->GetTarget(static_begin + i);
                const auto dynamic_target = dynamic_graph->GetTarget(dynamic_begin + i);
                BOOST_ASSERT(static_target == dynamic_target);

                const auto static_data = static_graph->GetEdgeData(static_begin + i);
                const auto dynamic_data = dynamic_graph->GetEdgeData(dynamic_begin + i);

                BOOST_ASSERT(static_data.distance == dynamic_data.distance);
                BOOST_ASSERT(static_data.name_id == dynamic_data.name_id);
            }

            // check range interface
            std::vector<EdgeID> static_target_ids, dynamic_target_ids;
            std::vector<TarjanEdgeData> static_edge_data, dynamic_edge_data;
            for (const auto static_id : static_range)
            {
                static_target_ids.push_back(static_graph->GetTarget(static_id));
                static_edge_data.push_back(static_graph->GetEdgeData(static_id));
            }
            for (const auto dynamic_id : dynamic_range)
            {
                dynamic_target_ids.push_back(dynamic_graph->GetTarget(dynamic_id));
                dynamic_edge_data.push_back(dynamic_graph->GetEdgeData(dynamic_id));
            }
            BOOST_ASSERT(static_target_ids.size() == dynamic_target_ids.size());
            BOOST_ASSERT(std::equal(std::begin(static_target_ids), std::end(static_target_ids),
                                    std::begin(dynamic_target_ids)));
        }

        SimpleLogger().Write() << "Graph comparison finished successfully";
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[exception] " << e.what();
    }

    return 0;
}
