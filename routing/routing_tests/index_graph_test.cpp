#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing/routing_tests/index_graph_tools.hpp"

#include "routing_common/car_model.hpp"

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

void TestRoute(IndexGraphStarter::FakeVertex const & start,
               IndexGraphStarter::FakeVertex const & finish, size_t expectedLength,
               vector<Segment> const * expectedRoute, WorldGraph & graph)
{
  IndexGraphStarter starter(start, finish, graph);
  vector<Segment> route;
  double timeSec;
  auto const resultCode = CalculateRoute(starter, route, timeSec);
  TEST_EQUAL(resultCode, AStarAlgorithm<IndexGraphStarter>::Result::OK, ());

  TEST_GREATER(route.size(), 2, ());
  // Erase fake points.
  route.erase(route.begin());
  route.pop_back();

  TEST_EQUAL(route.size(), expectedLength, ("route =", route));

  if (expectedRoute)
    TEST_EQUAL(route, *expectedRoute, ());
}

void TestEdges(IndexGraph & graph, Segment const & segment, vector<Segment> const & expectedTargets,
               bool isOutgoing)
{
  ASSERT(segment.IsForward() || !graph.GetGeometry().GetRoad(segment.GetFeatureId()).IsOneWay(),
         ());

  vector<SegmentEdge> edges;
  graph.GetEdgeList(segment, isOutgoing, edges);

  vector<Segment> targets;
  for (SegmentEdge const & edge : edges)
    targets.push_back(edge.GetTarget());

  sort(targets.begin(), targets.end());

  vector<Segment> sortedExpectedTargets(expectedTargets);
  sort(sortedExpectedTargets.begin(), sortedExpectedTargets.end());

  TEST_EQUAL(targets, sortedExpectedTargets, ());
}

void TestOutgoingEdges(IndexGraph & graph, Segment const & segment,
                       vector<Segment> const & expectedTargets)
{
  TestEdges(graph, segment, expectedTargets, true /* isOutgoing */);
}

void TestIngoingEdges(IndexGraph & graph, Segment const & segment,
                      vector<Segment> const & expectedTargets)
{
  TestEdges(graph, segment, expectedTargets, false /* isOutgoing */);
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

  TestOutgoingEdges(
      graph, {kTestNumMwmId, 0 /* featureId */, 0 /* segmentIdx */, true /* forward */},
      {{kTestNumMwmId, 0, 0, false}, {kTestNumMwmId, 0, 1, true}, {kTestNumMwmId, 3, 1, true}});
  TestIngoingEdges(graph, {kTestNumMwmId, 0, 0, true}, {{kTestNumMwmId, 0, 0, false}});
  TestOutgoingEdges(graph, {kTestNumMwmId, 0, 0, false}, {{kTestNumMwmId, 0, 0, true}});
  TestIngoingEdges(
      graph, {kTestNumMwmId, 0, 0, false},
      {{kTestNumMwmId, 0, 0, true}, {kTestNumMwmId, 0, 1, false}, {kTestNumMwmId, 3, 0, true}});

  TestOutgoingEdges(
      graph, {kTestNumMwmId, 0, 2, true},
      {{kTestNumMwmId, 0, 2, false}, {kTestNumMwmId, 0, 3, true}, {kTestNumMwmId, 4, 1, true}});
  TestIngoingEdges(graph, {kTestNumMwmId, 0, 2, true}, {{kTestNumMwmId, 0, 1, true}});
  TestOutgoingEdges(graph, {kTestNumMwmId, 0, 2, false}, {{kTestNumMwmId, 0, 1, false}});
  TestIngoingEdges(
      graph, {kTestNumMwmId, 0, 2, false},
      {{kTestNumMwmId, 0, 2, true}, {kTestNumMwmId, 0, 3, false}, {kTestNumMwmId, 4, 0, true}});

  TestOutgoingEdges(
      graph, {kTestNumMwmId, 3, 0, true},
      {{kTestNumMwmId, 3, 1, true}, {kTestNumMwmId, 0, 0, false}, {kTestNumMwmId, 0, 1, true}});
  TestIngoingEdges(graph, {kTestNumMwmId, 3, 0, true}, {{kTestNumMwmId, 2, 0, false}});

  TestOutgoingEdges(graph, {kTestNumMwmId, 4, 1, true}, {{kTestNumMwmId, 2, 0, false}});
  TestIngoingEdges(
      graph, {kTestNumMwmId, 4, 1, true},
      {{kTestNumMwmId, 4, 0, true}, {kTestNumMwmId, 0, 2, true}, {kTestNumMwmId, 0, 3, false}});
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
      RoadGeometry::Points({{0.0, -2.0}, {0.0, -1.0}, {0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}}));

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimator(trafficCache);
  unique_ptr<WorldGraph> worldGraph =
      BuildWorldGraph(move(loader), estimator, {MakeJoint({{0, 2}, {1, 2}})});

  vector<IndexGraphStarter::FakeVertex> endPoints;
  for (uint32_t i = 0; i < 4; ++i)
  {
    endPoints.emplace_back(kTestNumMwmId, 0, i, m2::PointD(-1.5 + i, 0.0));
    endPoints.emplace_back(kTestNumMwmId, 1, i, m2::PointD(0.0, -1.5 + i));
  }

  for (auto const & start : endPoints)
  {
    for (auto const & finish : endPoints)
    {
      uint32_t expectedLength = 0;
      if (start.GetFeatureId() == finish.GetFeatureId())
      {
        expectedLength = AbsDelta(start.GetSegmentIdx(), finish.GetSegmentIdx()) + 1;
      }
      else
      {
        if (start.GetSegmentIdx() < 2)
          expectedLength += 2 - start.GetSegmentIdx();
        else
          expectedLength += start.GetSegmentIdx() - 1;

        if (finish.GetSegmentIdx() < 2)
          expectedLength += 2 - finish.GetSegmentIdx();
        else
          expectedLength += finish.GetSegmentIdx() - 1;
      }
      TestRoute(start, finish, expectedLength, nullptr, *worldGraph);
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
      street.emplace_back(static_cast<double>(j), static_cast<double>(i));
      avenue.emplace_back(static_cast<double>(i), static_cast<double>(j));
    }
    loader->AddRoad(i, false, 1.0 /* speed */, street);
    loader->AddRoad(i + kCitySize, false, 1.0 /* speed */, avenue);
  }

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimator(trafficCache);

  vector<Joint> joints;
  for (uint32_t i = 0; i < kCitySize; ++i)
  {
    for (uint32_t j = 0; j < kCitySize; ++j)
      joints.emplace_back(MakeJoint({{i, j}, {j + kCitySize, i}}));
  }

  unique_ptr<WorldGraph> worldGraph = BuildWorldGraph(move(loader), estimator, joints);

  vector<IndexGraphStarter::FakeVertex> endPoints;
  for (uint32_t featureId = 0; featureId < kCitySize; ++featureId)
  {
    for (uint32_t segmentId = 0; segmentId < kCitySize - 1; ++segmentId)
    {
      endPoints.emplace_back(kTestNumMwmId, featureId, segmentId,
                             m2::PointD(0.5 + segmentId, featureId));
      endPoints.emplace_back(kTestNumMwmId, featureId + kCitySize, segmentId,
                             m2::PointD(featureId, 0.5 + segmentId));
    }
  }

  for (auto const & start : endPoints)
  {
    for (auto const & finish : endPoints)
    {
      uint32_t expectedLength = 0;

      auto const startFeatureOffset = start.GetFeatureId() < kCitySize
                                          ? start.GetFeatureId()
                                          : start.GetFeatureId() - kCitySize;
      auto const finishFeatureOffset = finish.GetFeatureId() < kCitySize
                                           ? finish.GetFeatureId()
                                           : finish.GetFeatureId() - kCitySize;

      if (start.GetFeatureId() < kCitySize == finish.GetFeatureId() < kCitySize)
      {
        uint32_t segDelta = AbsDelta(start.GetSegmentIdx(), finish.GetSegmentIdx());
        if (segDelta == 0 && start.GetFeatureId() != finish.GetFeatureId())
          segDelta = 1;
        expectedLength += segDelta;
        expectedLength += AbsDelta(startFeatureOffset, finishFeatureOffset) + 1;
      }
      else
      {
        if (start.GetSegmentIdx() < finishFeatureOffset)
          expectedLength += finishFeatureOffset - start.GetSegmentIdx();
        else
          expectedLength += start.GetSegmentIdx() - finishFeatureOffset + 1;

        if (finish.GetSegmentIdx() < startFeatureOffset)
          expectedLength += startFeatureOffset - finish.GetSegmentIdx();
        else
          expectedLength += finish.GetSegmentIdx() - startFeatureOffset + 1;
      }

      TestRoute(start, finish, expectedLength, nullptr, *worldGraph);
    }
  }
}

// Roads                                          y:
//
//  fast road R0              * - * - *           -1
//                          /           \
//  slow road R1    * - -  *  - - * - -  * - - *   0
//                        J0            J1
//
//            x:    0      1  2   3   4  5     6
//
UNIT_TEST(RoadSpeed)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();

  loader->AddRoad(
      0 /* featureId */, false, 10.0 /* speed */,
      RoadGeometry::Points({{1.0, 0.0}, {2.0, -1.0}, {3.0, -1.0}, {4.0, -1.0}, {5.0, 0.0}}));

  loader->AddRoad(
      1 /* featureId */, false, 1.0 /* speed */,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}, {3.0, 0.0}, {5.0, 0.0}, {6.0, 0.0}}));

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimator(trafficCache);

  vector<Joint> joints;
  joints.emplace_back(MakeJoint({{0, 0}, {1, 1}}));  // J0
  joints.emplace_back(MakeJoint({{0, 4}, {1, 3}}));  // J1

  unique_ptr<WorldGraph> worldGraph = BuildWorldGraph(move(loader), estimator, joints);

  IndexGraphStarter::FakeVertex const start(kTestNumMwmId, 1, 0, m2::PointD(0.5, 0));
  IndexGraphStarter::FakeVertex const finish(kTestNumMwmId, 1, 3, m2::PointD(5.5, 0));

  vector<Segment> const expectedRoute({{kTestNumMwmId, 1, 0, true},
                                       {kTestNumMwmId, 0, 0, true},
                                       {kTestNumMwmId, 0, 1, true},
                                       {kTestNumMwmId, 0, 2, true},
                                       {kTestNumMwmId, 0, 3, true},
                                       {kTestNumMwmId, 1, 3, true}});
  TestRoute(start, finish, 6, &expectedRoute, *worldGraph);
}

// Roads                             y:
//
//    R0    * - - - - - - - - *      0
//                ^     ^
//             start   finish
//
//    x:    0     1     2     3
//
UNIT_TEST(OneSegmentWay)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();

  loader->AddRoad(0 /* featureId */, false, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {3.0, 0.0}}));

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimator(trafficCache);
  unique_ptr<WorldGraph> worldGraph = BuildWorldGraph(move(loader), estimator, vector<Joint>());

  IndexGraphStarter::FakeVertex const start(kTestNumMwmId, 0, 0, m2::PointD(1, 0));
  IndexGraphStarter::FakeVertex const finish(kTestNumMwmId, 0, 0, m2::PointD(2, 0));

  vector<Segment> const expectedRoute({{kTestNumMwmId, 0, 0, true}});
  TestRoute(start, finish, 1 /* expectedLength */, &expectedRoute, *worldGraph);
}

//
//  Road       R0 (ped)       R1 (car)       R2 (car)
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
    unordered_map<uint32_t, VehicleMask> masks;
    masks[0] = kPedestrianMask;
    masks[1] = kCarMask;
    masks[2] = kCarMask;

    MemWriter<vector<uint8_t>> writer(buffer);
    IndexGraphSerializer::Serialize(graph, masks, writer);
  }

  {
    IndexGraph graph;
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    IndexGraphSerializer::Deserialize(graph, source, kAllVehiclesMask);

    TEST_EQUAL(graph.GetNumRoads(), 3, ());
    TEST_EQUAL(graph.GetNumJoints(), 2, ());
    TEST_EQUAL(graph.GetNumPoints(), 4, ());

    TEST_EQUAL(graph.GetJointId({0, 0}), Joint::kInvalidId, ());
    TEST_EQUAL(graph.GetJointId({0, 1}), 1, ());
    TEST_EQUAL(graph.GetJointId({1, 0}), 1, ());
    TEST_EQUAL(graph.GetJointId({1, 1}), 0, ());
    TEST_EQUAL(graph.GetJointId({2, 0}), 0, ());
    TEST_EQUAL(graph.GetJointId({2, 1}), Joint::kInvalidId, ());
  }

  {
    IndexGraph graph;
    MemReader reader(buffer.data(), buffer.size());
    ReaderSource<MemReader> source(reader);
    IndexGraphSerializer::Deserialize(graph, source, kCarMask);

    TEST_EQUAL(graph.GetNumRoads(), 2, ());
    TEST_EQUAL(graph.GetNumJoints(), 1, ());
    TEST_EQUAL(graph.GetNumPoints(), 2, ());

    TEST_EQUAL(graph.GetJointId({0, 0}), Joint::kInvalidId, ());
    TEST_EQUAL(graph.GetJointId({0, 1}), Joint::kInvalidId, ());
    TEST_EQUAL(graph.GetJointId({1, 0}), Joint::kInvalidId, ());
    TEST_EQUAL(graph.GetJointId({1, 1}), 0, ());
    TEST_EQUAL(graph.GetJointId({2, 0}), 0, ());
    TEST_EQUAL(graph.GetJointId({2, 1}), Joint::kInvalidId, ());
  }
}

//      Finish
// 0.0004    *
//           ^
//           |
//           F2
//           |
//           |
// 0.0003    *---------*
//           |         |
//           |         |
//           |         |
//           |         |
//           |         |
// 0.0002    *         *
//           |         |
//           |         |
// 0.00015   *    F0   *
//            \       /
//              \   /
// 0.0001         *---F0-*----*
//                            ^
//                            |
//                            F1
//                            |
//                            |
// 0                          * Start
//   0         0.0001       0.0002
// F0 is a two-way feature with a loop and F1 and F2 are an one-way one-segment features.
unique_ptr<WorldGraph> BuildLoopGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* feature id */, false /* one way */, 100.0 /* speed */,
                  RoadGeometry::Points({{0.0002, 0.0001},
                                        {0.00015, 0.0001},
                                        {0.0001, 0.0001},
                                        {0.00015, 0.00015},
                                        {0.00015, 0.0002},
                                        {0.00015, 0.0003},
                                        {0.00005, 0.0003},
                                        {0.00005, 0.0002},
                                        {0.00005, 0.00015},
                                        {0.0001, 0.0001}}));
  loader->AddRoad(1 /* feature id */, true /* one way */, 100.0 /* speed */,
                  RoadGeometry::Points({{0.0002, 0.0}, {0.0002, 0.0001}}));
  loader->AddRoad(2 /* feature id */, true /* one way */, 100.0 /* speed */,
                  RoadGeometry::Points({{0.00005, 0.0003}, {0.00005, 0.0004}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 2 /* point id */}, {0, 9}}), /* joint at point (0.0002, 0) */
      MakeJoint({{1, 1}, {0, 0}}), /* joint at point (0.0002, 0.0001) */
      MakeJoint({{0, 6}, {2, 0}}), /* joint at point (0.00005, 0.0003) */
      MakeJoint({{2, 1}}),         /* joint at point (0.00005, 0.0004) */
  };

  traffic::TrafficCache const trafficCache;
  shared_ptr<EdgeEstimator> estimator = CreateEstimator(trafficCache);
  return BuildWorldGraph(move(loader), estimator, joints);
}

// This test checks that the route from Start to Finish doesn't make an extra loop in F0.
// If it was so the route time had been much more.
UNIT_CLASS_TEST(RestrictionTest, LoopGraph)
{
  Init(BuildLoopGraph());
  SetStarter(routing::IndexGraphStarter::FakeVertex(kTestNumMwmId, 1, 0 /* seg id */,
                                                    m2::PointD(0.0002, 0)) /* start */,
             routing::IndexGraphStarter::FakeVertex(kTestNumMwmId, 2, 0 /* seg id */,
                                                    m2::PointD(0.00005, 0.0004)) /* finish */);

  double constexpr kExpectedRouteTimeSec = 3.48;
  TestRouteTime(*m_starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, kExpectedRouteTimeSec);
}
}  // namespace routing_test
