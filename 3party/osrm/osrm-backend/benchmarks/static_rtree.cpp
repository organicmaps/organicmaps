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

#include "../data_structures/original_edge_data.hpp"
#include "../data_structures/query_node.hpp"
#include "../data_structures/shared_memory_vector_wrapper.hpp"
#include "../data_structures/static_rtree.hpp"
#include "../util/boost_filesystem_2_fix.hpp"
#include "../data_structures/edge_based_node.hpp"

#include <osrm/coordinate.hpp>

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

    QueryNode current_node;
    unsigned coordinate_count = 0;
    nodes_input_stream.read((char *)&coordinate_count, sizeof(unsigned));
    auto coords = std::make_shared<std::vector<FixedPointCoordinate>>(coordinate_count);
    for (unsigned i = 0; i < coordinate_count; ++i)
    {
        nodes_input_stream.read((char *)&current_node, sizeof(QueryNode));
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

    {
        const unsigned num_results = 5;
        std::cout << "#### IncrementalFindPhantomNodeForCoordinate : " << num_results
                  << " phantom nodes"
                  << "\n";

        TIMER_START(query_phantom);
        std::vector<PhantomNode> phantom_node_vector;
        for (const auto &q : queries)
        {
            phantom_node_vector.clear();
            rtree.IncrementalFindPhantomNodeForCoordinate(q, phantom_node_vector, 3, num_results);
            phantom_node_vector.clear();
            rtree.IncrementalFindPhantomNodeForCoordinate(q, phantom_node_vector, 17, num_results);
        }
        TIMER_STOP(query_phantom);

        std::cout << "Took " << TIMER_MSEC(query_phantom) << " msec for " << num_queries
                  << " queries."
                  << "\n";
        std::cout << TIMER_MSEC(query_phantom) / ((double)num_queries) << " msec/query."
                  << "\n";

        std::cout << "#### LocateClosestEndPointForCoordinate"
                  << "\n";
    }

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

    TIMER_START(query_node);
    for (const auto &q : queries)
    {
        PhantomNode phantom;
        rtree.FindPhantomNodeForCoordinate(q, phantom, 3);
    }
    TIMER_STOP(query_node);

    std::cout << "Took " << TIMER_MSEC(query_node) << " msec for " << num_queries << " queries."
              << "\n";
    std::cout << TIMER_MSEC(query_node) / ((double)num_queries) << " msec/query."
              << "\n";

    {
        const unsigned num_results = 1;
        std::cout << "#### IncrementalFindPhantomNodeForCoordinate : " << num_results
                  << " phantom nodes"
                  << "\n";

        TIMER_START(query_phantom);
        std::vector<PhantomNode> phantom_node_vector;
        for (const auto &q : queries)
        {
            phantom_node_vector.clear();
            rtree.IncrementalFindPhantomNodeForCoordinate(q, phantom_node_vector, 3, num_results);
            phantom_node_vector.clear();
            rtree.IncrementalFindPhantomNodeForCoordinate(q, phantom_node_vector, 17, num_results);
        }
        TIMER_STOP(query_phantom);

        std::cout << "Took " << TIMER_MSEC(query_phantom) << " msec for " << num_queries
                  << " queries."
                  << "\n";
        std::cout << TIMER_MSEC(query_phantom) / ((double)num_queries) << " msec/query."
                  << "\n";

        std::cout << "#### LocateClosestEndPointForCoordinate"
                  << "\n";
    }
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
