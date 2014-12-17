#include "../DataStructures/OriginalEdgeData.h"
#include "../DataStructures/QueryNode.h"
#include "../DataStructures/SharedMemoryVectorWrapper.h"
#include "../DataStructures/StaticRTree.h"
#include "../Util/BoostFileSystemFix.h"
#include "../DataStructures/EdgeBasedNode.h"

#include <osrm/Coordinate.h>

#include <random>

// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 13;
constexpr int32_t WORLD_MIN_LAT = -90 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MAX_LAT = 90 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MIN_LON = -180 * COORDINATE_PRECISION;
constexpr int32_t WORLD_MAX_LON = 180 * COORDINATE_PRECISION;

using RTreeLeaf = EdgeBasedNode;
using FixedPointCoordinateListPtr = std::shared_ptr<std::vector<FixedPointCoordinate>>;
using BenchStaticRTree = StaticRTree<RTreeLeaf, ShM<FixedPointCoordinate, false>::vector, false>;

FixedPointCoordinateListPtr LoadCoordinates(const boost::filesystem::path &nodes_file)
{
    boost::filesystem::ifstream nodes_input_stream(nodes_file, std::ios::binary);

    NodeInfo current_node;
    unsigned number_of_coordinates = 0;
    nodes_input_stream.read((char *)&number_of_coordinates, sizeof(unsigned));
    auto coords = std::make_shared<std::vector<FixedPointCoordinate>>(number_of_coordinates);
    for (unsigned i = 0; i < number_of_coordinates; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(NodeInfo));
        coords->at(i) = FixedPointCoordinate(current_node.lat, current_node.lon);
        BOOST_ASSERT((std::abs(coords->at(i).lat) >> 30) == 0);
        BOOST_ASSERT((std::abs(coords->at(i).lon) >> 30) == 0);
    }
    nodes_input_stream.close();
    return coords;
}

void Benchmark(BenchStaticRTree &rtree, unsigned num_queries)
{
    std::mt19937 mt_rand(RANDOM_SEED);
    std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
    std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);
    std::vector<FixedPointCoordinate> queries;
    for (unsigned i = 0; i < num_queries; i++)
    {
        queries.emplace_back(FixedPointCoordinate(lat_udist(mt_rand), lon_udist(mt_rand)));
    }

    const unsigned num_results = 5;
    std::cout << "#### IncrementalFindPhantomNodeForCoordinate : " << num_results
              << " phantom nodes"
              << "\n";

    TIMER_START(query_phantom);
    std::vector<PhantomNode> resulting_phantom_node_vector;
    for (const auto &q : queries)
    {
        resulting_phantom_node_vector.clear();
        rtree.IncrementalFindPhantomNodeForCoordinate(
            q, resulting_phantom_node_vector, 3, num_results);
        resulting_phantom_node_vector.clear();
        rtree.IncrementalFindPhantomNodeForCoordinate(
            q, resulting_phantom_node_vector, 17, num_results);
    }
    TIMER_STOP(query_phantom);

    std::cout << "Took " << TIMER_MSEC(query_phantom) << " msec for " << num_queries << " queries."
              << "\n";
    std::cout << TIMER_MSEC(query_phantom) / ((double)num_queries) << " msec/query."
              << "\n";

    std::cout << "#### LocateClosestEndPointForCoordinate"
              << "\n";

    TIMER_START(query_endpoint);
    FixedPointCoordinate result;
    for (const auto &q : queries)
    {
        rtree.LocateClosestEndPointForCoordinate(q, result, 3);
    }
    TIMER_STOP(query_endpoint);

    std::cout << "Took " << TIMER_MSEC(query_endpoint) << " msec for " << num_queries << " queries."
              << "\n";
    std::cout << TIMER_MSEC(query_endpoint) / ((double)num_queries) << " msec/query."
              << "\n";

    std::cout << "#### FindPhantomNodeForCoordinate"
              << "\n";

    TIMER_START(query_phantomnode);
    for (const auto &q : queries)
    {
        PhantomNode phantom;
        rtree.FindPhantomNodeForCoordinate(q, phantom, 3);
    }
    TIMER_STOP(query_phantomnode);

    std::cout << "Took " << TIMER_MSEC(query_phantomnode) << " msec for " << num_queries
              << " queries."
              << "\n";
    std::cout << TIMER_MSEC(query_phantomnode) / ((double)num_queries) << " msec/query."
              << "\n";
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cout << "./rtree-bench file.ramIndex file.fileIndx file.nodes"
                  << "\n";
        return 1;
    }

    const char *ramPath = argv[1];
    const char *filePath = argv[2];
    const char *nodesPath = argv[3];

    auto coords = LoadCoordinates(nodesPath);

    BenchStaticRTree rtree(ramPath, filePath, coords);

    Benchmark(rtree, 10000);

    return 0;
}
