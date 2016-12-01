#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/car_model.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_serializer.hpp"
#include "routing/index_graph_starter.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "geometry/point2d.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

namespace
{
using namespace routing;
using namespace routing_test;

void TestRoute(IndexGraph & graph, RoadPoint const & start, RoadPoint const & finish,
               size_t expectedLength, vector<RoadPoint> const * expectedRoute = nullptr)
{
  LOG(LINFO, ("Test route", start.GetFeatureId(), ",", start.GetPointId(), "=>",
              finish.GetFeatureId(), ",", finish.GetPointId()));

  IndexGraphStarter starter(graph, start, finish);
  vector<RoadPoint> roadPoints;
  auto const resultCode = CalculateRoute(starter, roadPoints);
  TEST_EQUAL(resultCode, AStarAlgorithm<IndexGraphStarter>::Result::OK, ());

  TEST_EQUAL(roadPoints.size(), expectedLength, ());

  if (expectedRoute)
    TEST_EQUAL(roadPoints, *expectedRoute, ());
}

void TestEdges(IndexGraph & graph, Joint::Id jointId, vector<Joint::Id> const & expectedTargets,
               bool forward)
{
  vector<JointEdge> edges;
  graph.GetEdgeList(jointId, forward, edges);

  vector<Joint::Id> targets;
  for (JointEdge const & edge : edges)
    targets.push_back(edge.GetTarget());

  sort(targets.begin(), targets.end());

  TEST_EQUAL(targets, expectedTargets, ());
}

void TestOutgoingEdges(IndexGraph & graph, Joint::Id jointId,
                       vector<Joint::Id> const & expectedTargets)
{
  TestEdges(graph, jointId, expectedTargets, true);
}

void TestIngoingEdges(IndexGraph & graph, Joint::Id jointId,
                      vector<Joint::Id> const & expectedTargets)
{
  TestEdges(graph, jointId, expectedTargets, false);
}

uint32_t AbsDelta(uint32_t v0, uint32_t v1) { return v0 > v1 ? v0 - v1 : v1 - v0; }
}  // namespace

namespace routing_test
{
//                   R4 (one way down)
//
// R1     J2--------J3         -1
//         ^         v
//         ^         v
// R0 *---J0----*---J1----*     0
//         ^         v
//         ^         v
// R2     J4--------J5          1
//
//        R3 (one way up)       y
//
// x: 0    1    2    3    4
//
UNIT_TEST(EdgesTest)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(
      0 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}}));
  loader->AddRoad(1 /* featureId */, false, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, -1.0}, {3.0, -1.0}}));
  loader->AddRoad(2 /* featureId */, false, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, -1.0}, {3.0, -1.0}}));
  loader->AddRoad(3 /* featureId */, true, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {1.0, 0.0}, {1.0, -1.0}}));
  loader->AddRoad(4 /* featureId */, true, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, -1.0}, {3.0, 0.0}, {3.0, 1.0}}));

  traffic::TrafficCache const trafficCache;
  IndexGraph graph(move(loader), CreateEstimator(trafficCache));

  vector<Joint> joints;
  joints.emplace_back(MakeJoint({{0, 1}, {3, 1}}));  // J0
  joints.emplace_back(MakeJoint({{0, 3}, {4, 1}}));  // J1
  joints.emplace_back(MakeJoint({{1, 0}, {3, 2}}));  // J2
  joints.emplace_back(MakeJoint({{1, 1}, {4, 0}}));  // J3
  joints.emplace_back(MakeJoint({{2, 0}, {3, 0}}));  // J4
  joints.emplace_back(MakeJoint({{2, 1}, {4, 2}}));  // J5
  graph.Import(joints);

  TestOutgoingEdges(graph, 0, {1, 2});
  TestOutgoingEdges(graph, 1, {0, 5});
  TestOutgoingEdges(graph, 2, {3});
  TestOutgoingEdges(graph, 3, {1, 2});
  TestOutgoingEdges(graph, 4, {0, 5});
  TestOutgoingEdges(graph, 5, {4});

  TestIngoingEdges(graph, 0, {1, 4});
  TestIngoingEdges(graph, 1, {0, 3});
  TestIngoingEdges(graph, 2, {0, 3});
  TestIngoingEdges(graph, 3, {2});
  TestIngoingEdges(graph, 4, {5});
  TestIngoingEdges(graph, 5, {1, 4});
}

//  Roads     R1:
//
//            -2
//            -1
//  R0  -2 -1  0  1  2
//             1
//             2
//
UNIT_TEST(FindPathCross)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(
      0 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{-2.0, 0.0}, {-1.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(
      1 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{0.0, -2.0}, {-1.0, 0.0}, {0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}}));

  traffic::TrafficCache const trafficCache;
  IndexGraph graph(move(loader), CreateEstimator(trafficCache));

  graph.Import({MakeJoint({{0, 2}, {1, 2}})});

  vector<RoadPoint> points;
  for (uint32_t i = 0; i < 5; ++i)
  {
    points.emplace_back(0, i);
    points.emplace_back(1, i);
  }

  for (auto const & start : points)
  {
    for (auto const & finish : points)
    {
      uint32_t expectedLength;
      // Length of the route is the number of route points.
      // Example: p0 --- p1 --- p2
      // 2 segments, 3 points,
      // Therefore route length = geometrical length + 1
      if (start.GetFeatureId() == finish.GetFeatureId())
        expectedLength = AbsDelta(start.GetPointId(), finish.GetPointId()) + 1;
      else
        expectedLength = AbsDelta(start.GetPointId(), 2) + AbsDelta(finish.GetPointId(), 2) + 1;
      TestRoute(graph, start, finish, expectedLength);
    }
  }
}

// Roads   R4  R5  R6  R7
//
//    R0   0 - * - * - *
//         |   |   |   |
//    R1   * - 1 - * - *
//         |   |   |   |
//    R2   * - * - 2 - *
//         |   |   |   |
//    R3   * - * - * - 3
//
UNIT_TEST(FindPathManhattan)
{
  uint32_t constexpr kCitySize = 4;
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  for (uint32_t i = 0; i < kCitySize; ++i)
  {
    RoadGeometry::Points street;
    RoadGeometry::Points avenue;
    for (uint32_t j = 0; j < kCitySize; ++j)
    {
      street.emplace_back(static_cast<double>(i), static_cast<double>(j));
      avenue.emplace_back(static_cast<double>(j), static_cast<double>(i));
    }
    loader->AddRoad(i, false, 1.0 /* speed */, street);
    loader->AddRoad(i + kCitySize, false, 1.0 /* speed */, avenue);
  }

  traffic::TrafficCache const trafficCache;
  IndexGraph graph(move(loader), CreateEstimator(trafficCache));

  vector<Joint> joints;
  for (uint32_t i = 0; i < kCitySize; ++i)
  {
    for (uint32_t j = 0; j < kCitySize; ++j)
      joints.emplace_back(MakeJoint({{i, j}, {j + kCitySize, i}}));
  }
  graph.Import(joints);

  for (uint32_t startY = 0; startY < kCitySize; ++startY)
  {
    for (uint32_t startX = 0; startX < kCitySize; ++startX)
    {
      for (uint32_t finishY = 0; finishY < kCitySize; ++finishY)
      {
        for (uint32_t finishX = 0; finishX < kCitySize; ++finishX)
          TestRoute(graph, {startX, startY}, {finishX, finishY},
                    AbsDelta(startX, finishX) + AbsDelta(startY, finishY) + 1);
      }
    }
  }
}

// Roads                          y:
//
//  R0:         * - * - *         -1
//            /           \
//  R1:   J0 * -* - * - *- * J1    0
//            \           /
//  R2:         * - * - *          1
//
//     x:    0  1   2   3  4
//
UNIT_TEST(RedressRace)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();

  loader->AddRoad(
      0 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, -1.0}, {2.0, -1.0}, {3.0, -1.0}, {4.0, 0.0}}));

  loader->AddRoad(
      1 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}}));

  loader->AddRoad(
      2 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, 1.0}, {2.0, 1.0}, {3.0, 1.0}, {4.0, 0.0}}));

  traffic::TrafficCache const trafficCache;
  IndexGraph graph(move(loader), CreateEstimator(trafficCache));

  vector<Joint> joints;
  joints.emplace_back(MakeJoint({{0, 0}, {1, 0}, {2, 0}}));  // J0
  joints.emplace_back(MakeJoint({{0, 4}, {1, 4}, {2, 4}}));  // J1
  graph.Import(joints);

  vector<RoadPoint> const expectedRoute({{1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}});
  TestRoute(graph, {0, 0}, {0, 4}, 5, &expectedRoute);
}

// Roads                                       y:
//
//  fast road R0            * - * - *         -1
//                        /           \
//  slow road R1      J0 *  - - * - -  * J1    0
//
//                 x:    0  1   2   3  4
//
UNIT_TEST(RoadSpeed)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();

  loader->AddRoad(
      0 /* featureId */, false, 10.0 /* speed */,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, -1.0}, {2.0, -1.0}, {3.0, -1.0}, {4.0, 0.0}}));

  loader->AddRoad(1 /* featureId */, false, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {2.0, 0.0}, {4.0, 0.0}}));

  traffic::TrafficCache const trafficCache;
  IndexGraph graph(move(loader), CreateEstimator(trafficCache));

  vector<Joint> joints;
  joints.emplace_back(MakeJoint({{0, 0}, {1, 0}}));  // J0
  joints.emplace_back(MakeJoint({{0, 4}, {1, 2}}));  // J1
  graph.Import(joints);

  vector<RoadPoint> const expectedRoute({{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}});
  TestRoute(graph, {0, 0}, {0, 4}, 5, &expectedRoute);
}

//
//  Road       R0 (all)       R1 (car)       R2 (car)
//           0----------1 * 0----------1 * 0----------1
//  Joints               J0             J1
//
UNIT_TEST(SerializeSimpleGraph)
{
  vector<uint8_t> buffer;
  {
    IndexGraph graph;
    vector<Joint> joints = {
        MakeJoint({{0, 1}, {1, 0}}), MakeJoint({{1, 1}, {2, 0}}),
    };
    graph.Import(joints);
    unordered_set<uint32_t> const carFeatureIds = {1, 2};

    MemWriter<vector<uint8_t>> writer(buffer);
    IndexGraphSerializer::Serialize(graph, carFeatureIds, writer);
  }

  {
    IndexGraph graph;
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    IndexGraphSerializer::Deserialize(graph, source, false);

    TEST_EQUAL(graph.GetNumRoads(), 3, ());
    TEST_EQUAL(graph.GetNumJoints(), 2, ());
    TEST_EQUAL(graph.GetNumPoints(), 4, ());
  }

  {
    IndexGraph graph;
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    IndexGraphSerializer::Deserialize(graph, source, true);

    TEST_EQUAL(graph.GetNumRoads(), 2, ());
    TEST_EQUAL(graph.GetNumJoints(), 1, ());
    TEST_EQUAL(graph.GetNumPoints(), 2, ());
  }
}
}  // namespace routing_test
