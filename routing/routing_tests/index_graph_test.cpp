#include "testing/testing.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/car_model.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_starter.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

namespace
{
using namespace routing;

class TestGeometryLoader final : public GeometryLoader
{
public:
  // GeometryLoader overrides:
  void Load(uint32_t featureId, RoadGeometry & road) const override;

  void AddRoad(uint32_t featureId, bool oneWay, RoadGeometry::Points const & points);

private:
  unordered_map<uint32_t, RoadGeometry> m_roads;
};

void TestGeometryLoader::Load(uint32_t featureId, RoadGeometry & road) const
{
  auto it = m_roads.find(featureId);
  if (it == m_roads.cend())
    return;

  road = it->second;
}

void TestGeometryLoader::AddRoad(uint32_t featureId, bool oneWay,
                                 RoadGeometry::Points const & points)
{
  auto it = m_roads.find(featureId);
  if (it != m_roads.end())
  {
    ASSERT(false, ("Already contains feature", featureId));
    return;
  }

  m_roads[featureId] = RoadGeometry(oneWay, 1.0 /* speed */, points);
}

Joint MakeJoint(vector<RoadPoint> const & points)
{
  Joint joint;
  for (auto const & point : points)
    joint.AddPoint(point);

  return joint;
}

shared_ptr<EdgeEstimator> CreateEstimator()
{
  return CreateCarEdgeEstimator(make_shared<CarModelFactory>()->GetVehicleModel());
}

void TestRoute(IndexGraph const & graph, RoadPoint const & start, RoadPoint const & finish,
               size_t expectedLength)
{
  LOG(LINFO, ("Test route", start.GetFeatureId(), ",", start.GetPointId(), "=>",
              finish.GetFeatureId(), ",", finish.GetPointId()));

  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Joint::Id> routingResult;

  IndexGraphStarter starter(graph, start, finish);
  auto const resultCode = algorithm.FindPath(starter, starter.GetStartJoint(),
                                             starter.GetFinishJoint(), routingResult, {}, {});
  TEST_EQUAL(resultCode, AStarAlgorithm<IndexGraphStarter>::Result::OK, ());

  vector<RoadPoint> roadPoints;
  starter.RedressRoute(routingResult.path, roadPoints);
  TEST_EQUAL(roadPoints.size(), expectedLength, ());
}

void TestEdges(IndexGraph const & graph, Joint::Id jointId,
               vector<Joint::Id> const & expectedTargets, bool forward)
{
  vector<JointEdge> edges;
  graph.GetEdgesList(jointId, forward, edges);

  vector<Joint::Id> targets;
  for (JointEdge const & edge : edges)
    targets.push_back(edge.GetTarget());

  sort(targets.begin(), targets.end());

  ASSERT_EQUAL(targets, expectedTargets, ());
}

void TestOutgoingEdges(IndexGraph const & graph, Joint::Id jointId,
                       vector<Joint::Id> const & expectedTargets)
{
  TestEdges(graph, jointId, expectedTargets, true);
}

void TestIngoingEdges(IndexGraph const & graph, Joint::Id jointId,
                      vector<Joint::Id> const & expectedTargets)
{
  TestEdges(graph, jointId, expectedTargets, false);
}

uint32_t AbsDelta(uint32_t v0, uint32_t v1) { return v0 > v1 ? v0 - v1 : v1 - v0; }
}  // namespace

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
      0 /* featureId */, false,
      RoadGeometry::Points({{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}, {4.0, 0.0}}));
  loader->AddRoad(1 /* featureId */, false, RoadGeometry::Points({{1.0, -1.0}, {3.0, -1.0}}));
  loader->AddRoad(2 /* featureId */, false, RoadGeometry::Points({{1.0, -1.0}, {3.0, -1.0}}));
  loader->AddRoad(3 /* featureId */, true,
                  RoadGeometry::Points({{1.0, 1.0}, {1.0, 0.0}, {1.0, -1.0}}));
  loader->AddRoad(4 /* featureId */, true,
                  RoadGeometry::Points({{3.0, -1.0}, {3.0, 0.0}, {3.0, 1.0}}));

  IndexGraph graph(move(loader), CreateEstimator());

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

namespace routing_test
{
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
      0 /* featureId */, false,
      RoadGeometry::Points({{-2.0, 0.0}, {-1.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}}));
  loader->AddRoad(
      1 /* featureId */, false,
      RoadGeometry::Points({{0.0, -2.0}, {-1.0, 0.0}, {0.0, 0.0}, {0.0, 1.0}, {0.0, 2.0}}));

  IndexGraph graph(move(loader), CreateEstimator());

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
    loader->AddRoad(i, false, street);
    loader->AddRoad(i + kCitySize, false, avenue);
  }

  IndexGraph graph(move(loader), CreateEstimator());

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
}  // namespace routing_test
