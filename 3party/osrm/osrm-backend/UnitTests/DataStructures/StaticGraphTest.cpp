#include "../../DataStructures/StaticGraph.h"
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
    bool shortcut;
    unsigned distance;
};

struct TestEdge
{
    unsigned source;
    unsigned target;
    unsigned distance;
};

typedef StaticGraph<TestData> TestStaticGraph;
typedef TestStaticGraph::NodeArrayEntry TestNodeArrayEntry;
typedef TestStaticGraph::EdgeArrayEntry TestEdgeArrayEntry;
typedef TestStaticGraph::InputEdge TestInputEdge;

constexpr unsigned TEST_NUM_NODES = 100;
constexpr unsigned TEST_NUM_EDGES = 500;
// Choosen by a fair W20 dice roll (this value is completely arbitrary)
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
                TestEdgeArrayEntry{static_cast<unsigned>(node_udist(g)),
                                   TestData{i, false, static_cast<unsigned>(lengths_udist(g))}});
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

TestStaticGraph GraphFromEdgeList(const std::vector<TestEdge> &edges)
{
    std::vector<TestInputEdge> input_edges;
    unsigned i = 0;
    unsigned num_nodes = 0;
    for (const auto &e : edges)
    {
        input_edges.push_back(TestInputEdge{e.source, e.target, TestData{i++, false, e.distance}});

        num_nodes = std::max(num_nodes, std::max(e.source, e.target));
    }

    return TestStaticGraph(num_nodes, input_edges);
}

BOOST_AUTO_TEST_CASE(find_test)
{
    /*
     *  (0) -1-> (1)
     *  ^ ^
     *  2 1
     *  | |
     *  (3) -4-> (4)
     *      <-3-
     */
    TestStaticGraph simple_graph = GraphFromEdgeList({TestEdge{0, 1, 1},
                                                      TestEdge{3, 0, 2},
                                                      TestEdge{3, 4, 4},
                                                      TestEdge{4, 3, 3},
                                                      TestEdge{3, 0, 1}});

    auto eit = simple_graph.FindEdge(0, 1);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 0);

    eit = simple_graph.FindEdge(1, 0);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdgeInEitherDirection(1, 0);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 0);

    bool reverse = false;
    eit = simple_graph.FindEdgeIndicateIfReverse(1, 0, reverse);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 0);
    BOOST_CHECK(reverse);

    eit = simple_graph.FindEdge(3, 1);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);
    eit = simple_graph.FindEdge(0, 4);
    BOOST_CHECK_EQUAL(eit, SPECIAL_EDGEID);

    eit = simple_graph.FindEdge(3, 4);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 2);
    eit = simple_graph.FindEdgeInEitherDirection(3, 4);
    // I think this is wrong behaviour! Should be 3.
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 2);

    eit = simple_graph.FindEdge(3, 0);
    BOOST_CHECK_EQUAL(simple_graph.GetEdgeData(eit).id, 4);
}

BOOST_AUTO_TEST_SUITE_END()
