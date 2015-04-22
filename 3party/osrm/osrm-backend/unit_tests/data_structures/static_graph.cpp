/*

Copyright (c) 2014, Project OSRM contributors
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

#include "../../data_structures/static_graph.hpp"
#include "../../typedefs.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <random>
#include <unordered_map>

BOOST_AUTO_TEST_SUITE(static_graph)

struct TestData
{
    EdgeID id;
};

typedef StaticGraph<TestData> TestStaticGraph;
typedef TestStaticGraph::NodeArrayEntry TestNodeArrayEntry;
typedef TestStaticGraph::EdgeArrayEntry TestEdgeArrayEntry;
typedef TestStaticGraph::InputEdge TestInputEdge;

constexpr unsigned TEST_NUM_NODES = 100;
constexpr unsigned TEST_NUM_EDGES = 500;
// Chosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 15;

template <unsigned NUM_NODES, unsigned NUM_EDGES> struct RandomArrayEntryFixture
{
    RandomArrayEntryFixture()
    {
        std::mt19937 g(RANDOM_SEED);

        std::uniform_int_distribution<> edge_udist(0, NUM_EDGES - 1);
        std::vector<unsigned> offsets;
        for (unsigned i = 0; i < NUM_NODES; i++)
        {
            offsets.push_back(edge_udist(g));
        }
        std::sort(offsets.begin(), offsets.end());
        // add sentinel
        offsets.push_back(offsets.back());

        // extract interval lengths
        for (unsigned i = 0; i < offsets.size() - 1; i++)
        {
            lengths.push_back(offsets[i + 1] - offsets[i]);
        }
        lengths.push_back(NUM_EDGES - offsets[NUM_NODES - 1]);

        for (auto offset : offsets)
        {
            nodes.emplace_back(TestNodeArrayEntry{offset});
        }

        std::uniform_int_distribution<> lengths_udist(0, 100000);
        std::uniform_int_distribution<> node_udist(0, NUM_NODES - 1);
        for (unsigned i = 0; i < NUM_EDGES; i++)
        {
            edges.emplace_back(
                TestEdgeArrayEntry{static_cast<unsigned>(node_udist(g)), TestData{i}});
        }

        for (unsigned i = 0; i < NUM_NODES; i++)
            order.push_back(i);
        std::shuffle(order.begin(), order.end(), g);
    }

    typename ShM<TestNodeArrayEntry, false>::vector nodes;
    typename ShM<TestEdgeArrayEntry, false>::vector edges;
    std::vector<unsigned> lengths;
    std::vector<unsigned> order;
};

typedef RandomArrayEntryFixture<TEST_NUM_NODES, TEST_NUM_EDGES> TestRandomArrayEntryFixture;

BOOST_FIXTURE_TEST_CASE(array_test, TestRandomArrayEntryFixture)
{
    auto nodes_copy = nodes;

    TestStaticGraph graph(nodes, edges);

    BOOST_CHECK_EQUAL(graph.GetNumberOfEdges(), TEST_NUM_EDGES);
    BOOST_CHECK_EQUAL(graph.GetNumberOfNodes(), TEST_NUM_NODES);

    for (auto idx : order)
    {
        BOOST_CHECK_EQUAL(graph.BeginEdges((NodeID)idx), nodes_copy[idx].first_edge);
        BOOST_CHECK_EQUAL(graph.EndEdges((NodeID)idx), nodes_copy[idx + 1].first_edge);
        BOOST_CHECK_EQUAL(graph.GetOutDegree((NodeID)idx), lengths[idx]);
    }
}

BOOST_AUTO_TEST_CASE(find_test)
{
    /*
     *  (0) -1-> (1)
     *  ^ ^
     *  2 5
     *  | |
     *  (3) -3-> (4)
     *      <-4-
     */
    std::vector<TestInputEdge> input_edges = {
        TestInputEdge{0, 1, TestData{1}},
        TestInputEdge{3, 0, TestData{2}},
        TestInputEdge{3, 4, TestData{3}},
        TestInputEdge{4, 3, TestData{4}},
        TestInputEdge{3, 0, TestData{5}}
    };
    TestStaticGraph simple_graph(5, input_edges);

    auto eit = simple_graph.FindEdge(0, 1);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);

    eit = simple_graph.FindEdge(1, 0);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdgeInEitherDirection(1, 0);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);

    bool reverse = false;
    eit = simple_graph.FindEdgeIndicateIfReverse(1, 0, reverse);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 1);
    BOOST_CHECK(reverse);

    eit = simple_graph.FindEdge(3, 1);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);
    eit = simple_graph.FindEdge(0, 4);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdge(3, 4);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 3);
    eit = simple_graph.FindEdgeInEitherDirection(3, 4);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 3);

    eit = simple_graph.FindEdge(3, 0);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 2);
}

BOOST_AUTO_TEST_SUITE_END()
