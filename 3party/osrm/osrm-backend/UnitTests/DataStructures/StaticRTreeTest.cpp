#include "../../DataStructures/StaticRTree.h"
#include "../../DataStructures/QueryNode.h"
#include "../../DataStructures/EdgeBasedNode.h"
#include "../../Util/floating_point.hpp"
#include "../../typedefs.h"

#include <osrm/Coordinate.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <random>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(static_rtree)

constexpr uint32_t TEST_BRANCHING_FACTOR = 8;
constexpr uint32_t TEST_LEAF_NODE_SIZE = 64;

typedef EdgeBasedNode TestData;
typedef StaticRTree<TestData,
                    std::vector<FixedPointCoordinate>,
                    false,
                    TEST_BRANCHING_FACTOR,
                    TEST_LEAF_NODE_SIZE> TestStaticRTree;

// Choosen by a fair W20 dice roll (this value is completely arbitrary)
constexpr unsigned RANDOM_SEED = 15;
static const int32_t WORLD_MIN_LAT = -90 * COORDINATE_PRECISION;
static const int32_t WORLD_MAX_LAT = 90 * COORDINATE_PRECISION;
static const int32_t WORLD_MIN_LON = -180 * COORDINATE_PRECISION;
static const int32_t WORLD_MAX_LON = 180 * COORDINATE_PRECISION;

class LinearSearchNN
{
  public:
    LinearSearchNN(const std::shared_ptr<std::vector<FixedPointCoordinate>> &coords,
                   const std::vector<TestData> &edges)
        : coords(coords), edges(edges)
    {
    }

    bool LocateClosestEndPointForCoordinate(const FixedPointCoordinate &input_coordinate,
                                            FixedPointCoordinate &result_coordinate,
                                            const unsigned zoom_level)
    {
        bool ignore_tiny_components = (zoom_level <= 14);

        float min_dist = std::numeric_limits<float>::max();
        FixedPointCoordinate min_coord;
        for (const TestData &e : edges)
        {
            if (ignore_tiny_components && e.is_in_tiny_cc)
                continue;

            const FixedPointCoordinate &start = coords->at(e.u);
            const FixedPointCoordinate &end = coords->at(e.v);
            float distance = FixedPointCoordinate::ApproximateEuclideanDistance(
                input_coordinate.lat, input_coordinate.lon, start.lat, start.lon);
            if (distance < min_dist)
            {
                min_coord = start;
                min_dist = distance;
            }

            distance = FixedPointCoordinate::ApproximateEuclideanDistance(
                input_coordinate.lat, input_coordinate.lon, end.lat, end.lon);
            if (distance < min_dist)
            {
                min_coord = end;
                min_dist = distance;
            }
        }

        result_coordinate = min_coord;
        return result_coordinate.isValid();
    }

    bool FindPhantomNodeForCoordinate(const FixedPointCoordinate &input_coordinate,
                                      PhantomNode &result_phantom_node,
                                      const unsigned zoom_level)
    {
        bool ignore_tiny_components = (zoom_level <= 14);

        float min_dist = std::numeric_limits<float>::max();
        TestData nearest_edge;
        for (const TestData &e : edges)
        {
            if (ignore_tiny_components && e.is_in_tiny_cc)
                continue;

            float current_ratio = 0.;
            FixedPointCoordinate nearest;
            const float current_perpendicular_distance =
                FixedPointCoordinate::ComputePerpendicularDistance(
                    coords->at(e.u), coords->at(e.v), input_coordinate, nearest, current_ratio);

            if ((current_perpendicular_distance < min_dist) &&
                !osrm::epsilon_compare(current_perpendicular_distance, min_dist))
            { // found a new minimum
                min_dist = current_perpendicular_distance;
                result_phantom_node = {e.forward_edge_based_node_id,
                                       e.reverse_edge_based_node_id,
                                       e.name_id,
                                       e.forward_weight,
                                       e.reverse_weight,
                                       e.forward_offset,
                                       e.reverse_offset,
                                       e.packed_geometry_id,
                                       nearest,
                                       e.fwd_segment_position,
                                       e.forward_travel_mode,
                                       e.backward_travel_mode};
                nearest_edge = e;
            }
        }

        if (result_phantom_node.location.isValid())
        {
            // Hack to fix rounding errors and wandering via nodes.
            if (1 == std::abs(input_coordinate.lon - result_phantom_node.location.lon))
            {
                result_phantom_node.location.lon = input_coordinate.lon;
            }
            if (1 == std::abs(input_coordinate.lat - result_phantom_node.location.lat))
            {
                result_phantom_node.location.lat = input_coordinate.lat;
            }

            const float distance_1 = FixedPointCoordinate::ApproximateEuclideanDistance(
                coords->at(nearest_edge.u), result_phantom_node.location);
            const float distance_2 = FixedPointCoordinate::ApproximateEuclideanDistance(
                coords->at(nearest_edge.u), coords->at(nearest_edge.v));
            const float ratio = std::min(1.f, distance_1 / distance_2);

            if (SPECIAL_NODEID != result_phantom_node.forward_node_id)
            {
                result_phantom_node.forward_weight *= ratio;
            }
            if (SPECIAL_NODEID != result_phantom_node.reverse_node_id)
            {
                result_phantom_node.reverse_weight *= (1. - ratio);
            }
        }

        return result_phantom_node.location.isValid();
    }

  private:
    const std::shared_ptr<std::vector<FixedPointCoordinate>> &coords;
    const std::vector<TestData> &edges;
};

template <unsigned NUM_NODES, unsigned NUM_EDGES> struct RandomGraphFixture
{
    struct TupleHash
    {
        typedef std::pair<unsigned, unsigned> argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type &t) const
        {
            std::size_t val{0};
            boost::hash_combine(val, t.first);
            boost::hash_combine(val, t.second);
            return val;
        }
    };

    RandomGraphFixture() : coords(std::make_shared<std::vector<FixedPointCoordinate>>())
    {
        BOOST_TEST_MESSAGE("Constructing " << NUM_NODES << " nodes and " << NUM_EDGES << " edges.");

        std::mt19937 g(RANDOM_SEED);

        std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
        std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);

        for (unsigned i = 0; i < NUM_NODES; i++)
        {
            int lat = lat_udist(g);
            int lon = lon_udist(g);
            nodes.emplace_back(NodeInfo(lat, lon, i));
            coords->emplace_back(FixedPointCoordinate(lat, lon));
        }

        std::uniform_int_distribution<> edge_udist(0, nodes.size() - 1);

        std::unordered_set<std::pair<unsigned, unsigned>, TupleHash> used_edges;

        while (edges.size() < NUM_EDGES)
        {
            TestData data;
            data.u = edge_udist(g);
            data.v = edge_udist(g);
            if (used_edges.find(std::pair<unsigned, unsigned>(
                    std::min(data.u, data.v), std::max(data.u, data.v))) == used_edges.end())
            {
                data.is_in_tiny_cc = false;
                edges.emplace_back(data);
                used_edges.emplace(std::min(data.u, data.v), std::max(data.u, data.v));
            }
        }
    }

    std::vector<NodeInfo> nodes;
    std::shared_ptr<std::vector<FixedPointCoordinate>> coords;
    std::vector<TestData> edges;
};

struct GraphFixture
{
    GraphFixture(const std::vector<std::pair<float, float>> &input_coords,
                 const std::vector<std::pair<unsigned, unsigned>> &input_edges)
        : coords(std::make_shared<std::vector<FixedPointCoordinate>>())
    {

        for (unsigned i = 0; i < input_coords.size(); i++)
        {
            FixedPointCoordinate c(input_coords[i].first * COORDINATE_PRECISION,
                                   input_coords[i].second * COORDINATE_PRECISION);
            coords->emplace_back(c);
            nodes.emplace_back(NodeInfo(c.lat, c.lon, i));
        }

        for (const auto &pair : input_edges)
        {
            TestData d;
            d.u = pair.first;
            d.v = pair.second;
            edges.emplace_back(d);
        }
    }

    std::vector<NodeInfo> nodes;
    std::shared_ptr<std::vector<FixedPointCoordinate>> coords;
    std::vector<TestData> edges;
};

typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * 3, TEST_LEAF_NODE_SIZE / 2>
TestRandomGraphFixture_LeafHalfFull;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * 5, TEST_LEAF_NODE_SIZE>
TestRandomGraphFixture_LeafFull;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * 10, TEST_LEAF_NODE_SIZE * 2>
TestRandomGraphFixture_TwoLeaves;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR * 3,
                           TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR>
TestRandomGraphFixture_Branch;
typedef RandomGraphFixture<TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR * 3,
                           TEST_LEAF_NODE_SIZE * TEST_BRANCHING_FACTOR * 2>
TestRandomGraphFixture_MultipleLevels;

template <typename RTreeT>
void simple_verify_rtree(RTreeT &rtree,
                         const std::shared_ptr<std::vector<FixedPointCoordinate>> &coords,
                         const std::vector<TestData> &edges)
{
    BOOST_TEST_MESSAGE("Verify end points");
    for (const auto &e : edges)
    {
        FixedPointCoordinate result_u, result_v;
        const FixedPointCoordinate &pu = coords->at(e.u);
        const FixedPointCoordinate &pv = coords->at(e.v);
        bool found_u = rtree.LocateClosestEndPointForCoordinate(pu, result_u, 1);
        bool found_v = rtree.LocateClosestEndPointForCoordinate(pv, result_v, 1);
        BOOST_CHECK(found_u && found_v);
        float dist_u = FixedPointCoordinate::ApproximateEuclideanDistance(
            result_u.lat, result_u.lon, pu.lat, pu.lon);
        BOOST_CHECK_LE(dist_u, std::numeric_limits<float>::epsilon());
        float dist_v = FixedPointCoordinate::ApproximateEuclideanDistance(
            result_v.lat, result_v.lon, pv.lat, pv.lon);
        BOOST_CHECK_LE(dist_v, std::numeric_limits<float>::epsilon());
    }
}

template <typename RTreeT>
void sampling_verify_rtree(RTreeT &rtree, LinearSearchNN &lsnn, unsigned num_samples)
{
    std::mt19937 g(RANDOM_SEED);
    std::uniform_int_distribution<> lat_udist(WORLD_MIN_LAT, WORLD_MAX_LAT);
    std::uniform_int_distribution<> lon_udist(WORLD_MIN_LON, WORLD_MAX_LON);
    std::vector<FixedPointCoordinate> queries;
    for (unsigned i = 0; i < num_samples; i++)
    {
        queries.emplace_back(FixedPointCoordinate(lat_udist(g), lon_udist(g)));
    }

    BOOST_TEST_MESSAGE("Sampling queries");
    for (const auto &q : queries)
    {
        FixedPointCoordinate result_rtree;
        rtree.LocateClosestEndPointForCoordinate(q, result_rtree, 1);
        FixedPointCoordinate result_ln;
        lsnn.LocateClosestEndPointForCoordinate(q, result_ln, 1);
        BOOST_CHECK_EQUAL(result_ln, result_rtree);

        PhantomNode phantom_rtree;
        rtree.FindPhantomNodeForCoordinate(q, phantom_rtree, 1);
        PhantomNode phantom_ln;
        lsnn.FindPhantomNodeForCoordinate(q, phantom_ln, 1);
        BOOST_CHECK_EQUAL(phantom_rtree, phantom_ln);
    }
}

template <typename FixtureT, typename RTreeT = TestStaticRTree>
void build_rtree(const std::string &prefix,
                 FixtureT *fixture,
                 std::string &leaves_path,
                 std::string &nodes_path)
{
    nodes_path = prefix + ".ramIndex";
    leaves_path = prefix + ".fileIndex";
    const std::string coords_path = prefix + ".nodes";

    boost::filesystem::ofstream node_stream(coords_path, std::ios::binary);
    const unsigned num_nodes = fixture->nodes.size();
    node_stream.write((char *)&num_nodes, sizeof(unsigned));
    node_stream.write((char *)&(fixture->nodes[0]), num_nodes * sizeof(NodeInfo));
    node_stream.close();

    RTreeT r(fixture->edges, nodes_path, leaves_path, fixture->nodes);
}

template <typename FixtureT, typename RTreeT = TestStaticRTree>
void construction_test(const std::string &prefix, FixtureT *fixture)
{
    std::string leaves_path;
    std::string nodes_path;
    build_rtree<FixtureT, RTreeT>(prefix, fixture, leaves_path, nodes_path);
    RTreeT rtree(nodes_path, leaves_path, fixture->coords);
    LinearSearchNN lsnn(fixture->coords, fixture->edges);

    simple_verify_rtree(rtree, fixture->coords, fixture->edges);
    sampling_verify_rtree(rtree, lsnn, 100);
}

BOOST_FIXTURE_TEST_CASE(construct_half_leaf_test, TestRandomGraphFixture_LeafHalfFull)
{
    construction_test("test_1", this);
}

BOOST_FIXTURE_TEST_CASE(construct_full_leaf_test, TestRandomGraphFixture_LeafFull)
{
    construction_test("test_2", this);
}

BOOST_FIXTURE_TEST_CASE(construct_two_leaves_test, TestRandomGraphFixture_TwoLeaves)
{
    construction_test("test_3", this);
}

BOOST_FIXTURE_TEST_CASE(construct_branch_test, TestRandomGraphFixture_Branch)
{
    construction_test("test_4", this);
}

BOOST_FIXTURE_TEST_CASE(construct_multiple_levels_test, TestRandomGraphFixture_MultipleLevels)
{
    construction_test("test_5", this);
}

/*
 * Bug: If you querry a point that lies between two BBs that have a gap,
 * one BB will be pruned, even if it could contain a nearer match.
 */
BOOST_AUTO_TEST_CASE(regression_test)
{
    typedef std::pair<float, float> Coord;
    typedef std::pair<unsigned, unsigned> Edge;
    GraphFixture fixture({
                          Coord(40.0, 0.0),
                          Coord(35.0, 5.0),

                          Coord(5.0, 5.0),
                          Coord(0.0, 10.0),

                          Coord(20.0, 10.0),
                          Coord(20.0, 5.0),

                          Coord(40.0, 100.0),
                          Coord(35.0, 105.0),

                          Coord(5.0, 105.0),
                          Coord(0.0, 110.0),
                         },
                         {Edge(0, 1), Edge(2, 3), Edge(4, 5), Edge(6, 7), Edge(8, 9)});

    typedef StaticRTree<TestData, std::vector<FixedPointCoordinate>, false, 2, 3> MiniStaticRTree;

    std::string leaves_path;
    std::string nodes_path;
    build_rtree<GraphFixture, MiniStaticRTree>(
        "test_regression", &fixture, leaves_path, nodes_path);
    MiniStaticRTree rtree(nodes_path, leaves_path, fixture.coords);

    // query a node just right of the center of the gap
    FixedPointCoordinate input(20.0 * COORDINATE_PRECISION, 55.1 * COORDINATE_PRECISION);
    FixedPointCoordinate result;
    rtree.LocateClosestEndPointForCoordinate(input, result, 1);
    FixedPointCoordinate result_ln;
    LinearSearchNN lsnn(fixture.coords, fixture.edges);
    lsnn.LocateClosestEndPointForCoordinate(input, result_ln, 1);

    BOOST_CHECK_EQUAL(result_ln, result);
}

void TestRectangle(double width, double height, double center_lat, double center_lon)
{
    FixedPointCoordinate center(center_lat * COORDINATE_PRECISION,
                                center_lon * COORDINATE_PRECISION);

    TestStaticRTree::RectangleT rect;
    rect.min_lat = center.lat - height / 2.0 * COORDINATE_PRECISION;
    rect.max_lat = center.lat + height / 2.0 * COORDINATE_PRECISION;
    rect.min_lon = center.lon - width / 2.0 * COORDINATE_PRECISION;
    rect.max_lon = center.lon + width / 2.0 * COORDINATE_PRECISION;

    unsigned offset = 5 * COORDINATE_PRECISION;
    FixedPointCoordinate north(rect.max_lat + offset, center.lon);
    FixedPointCoordinate south(rect.min_lat - offset, center.lon);
    FixedPointCoordinate west(center.lat, rect.min_lon - offset);
    FixedPointCoordinate east(center.lat, rect.max_lon + offset);
    FixedPointCoordinate north_east(rect.max_lat + offset, rect.max_lon + offset);
    FixedPointCoordinate north_west(rect.max_lat + offset, rect.min_lon - offset);
    FixedPointCoordinate south_east(rect.min_lat - offset, rect.max_lon + offset);
    FixedPointCoordinate south_west(rect.min_lat - offset, rect.min_lon - offset);

    /* Distance to line segments of rectangle */
    BOOST_CHECK_EQUAL(rect.GetMinDist(north),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          north, FixedPointCoordinate(rect.max_lat, north.lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(south),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          south, FixedPointCoordinate(rect.min_lat, south.lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(west),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          west, FixedPointCoordinate(west.lat, rect.min_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(east),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          east, FixedPointCoordinate(east.lat, rect.max_lon)));

    /* Distance to corner points */
    BOOST_CHECK_EQUAL(rect.GetMinDist(north_east),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          north_east, FixedPointCoordinate(rect.max_lat, rect.max_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(north_west),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          north_west, FixedPointCoordinate(rect.max_lat, rect.min_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(south_east),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          south_east, FixedPointCoordinate(rect.min_lat, rect.max_lon)));
    BOOST_CHECK_EQUAL(rect.GetMinDist(south_west),
                      FixedPointCoordinate::ApproximateEuclideanDistance(
                          south_west, FixedPointCoordinate(rect.min_lat, rect.min_lon)));
}

BOOST_AUTO_TEST_CASE(rectangle_test)
{
    TestRectangle(10, 10, 5, 5);
    TestRectangle(10, 10, -5, 5);
    TestRectangle(10, 10, 5, -5);
    TestRectangle(10, 10, -5, -5);
    TestRectangle(10, 10, 0, 0);
}

BOOST_AUTO_TEST_SUITE_END()
