#include "testing/testing.hpp"

#include "routing/car_model.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_starter.hpp"

#include "traffic/traffic_info.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/point2d.hpp"

#include "routing/base/astar_algorithm.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace
{
using namespace routing;
using namespace traffic;

// @TODO(bykoianko) When PR with applying restricions is merged all the helper method
// should be used from "routing/routing_tests/index_graph_tools.hpp".
class TestGeometryLoader final : public routing::GeometryLoader
{
public:
  // GeometryLoader overrides:
  void Load(uint32_t featureId, routing::RoadGeometry & road) const override;

  void AddRoad(uint32_t featureId, bool oneWay, float speed,
               routing::RoadGeometry::Points const & points);

private:
  unordered_map<uint32_t, routing::RoadGeometry> m_roads;
};

void TestGeometryLoader::Load(uint32_t featureId, RoadGeometry & road) const
{
  auto it = m_roads.find(featureId);
  if (it == m_roads.cend())
    return;

  road = it->second;
}

void TestGeometryLoader::AddRoad(uint32_t featureId, bool oneWay, float speed,
                                 RoadGeometry::Points const & points)
{
  auto it = m_roads.find(featureId);
  CHECK(it == m_roads.end(), ("Already contains feature", featureId));
  m_roads[featureId] = RoadGeometry(oneWay, speed, points);
}

Joint MakeJoint(vector<RoadPoint> const & points)
{
  Joint joint;
  for (auto const & point : points)
    joint.AddPoint(point);

  return joint;
}

AStarAlgorithm<IndexGraphStarter>::Result CalculateRoute(IndexGraphStarter & starter,
                                                         vector<RoadPoint> & roadPoints)
{
  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Joint::Id> routingResult;
  auto const resultCode = algorithm.FindPath(starter, starter.GetStartJoint(),
                                             starter.GetFinishJoint(), routingResult, {}, {});

  starter.RedressRoute(routingResult.path, roadPoints);
  return resultCode;
}

void TestRouteGeometry(IndexGraphStarter & starter,
                       AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                       vector<m2::PointD> const & expectedRouteGeom)
{
  vector<RoadPoint> route;
  auto const resultCode = CalculateRoute(starter, route);
  TEST_EQUAL(resultCode, expectedRouteResult, ());
  TEST_EQUAL(route.size(), expectedRouteGeom.size(), ());
  for (size_t i = 0; i < route.size(); ++i)
  {
    RoadGeometry roadGeom = starter.GetGraph().GetGeometry().GetRoad(route[i].GetFeatureId());
    CHECK_LESS(route[i].GetPointId(), roadGeom.GetPointsCount(), ());
    TEST_EQUAL(expectedRouteGeom[i], roadGeom.GetPoint(route[i].GetPointId()), ());
  }
}

// @TODO(bykoianko) When PR with applying restricions is merged BuildXXGraph()
// should be moved to "routing/routing_tests/index_graph_tools.hpp" and it should
// be used by applying_traffic_test.cpp and cumulative_restriction_test.cpp.

//                        Finish
//  3        *              *
//             ↖          ↗
//              F5     F6
//                ↖   ↗
// 2 *              *
//    ↖          ↗   ↖
//      F2      F3      F4
//        ↖  ↗           ↖
// 1        *               *
//        ↗  ↖             ^
//      F0      F1          F8
//    ↗          ↖         |
// 0 *              *--F7--->*
//   0       1      2       3
//                Start
// Note. This graph contains of 9 one segment directed features.
unique_ptr<IndexGraph> BuildXXGraph(shared_ptr<EdgeEstimator> estimator)
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();
  loader->AddRoad(0 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{0.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(1 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {1.0, 1.0}}));
  loader->AddRoad(2 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {0.0, 2.0}}));
  loader->AddRoad(3 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{1.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(4 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 1.0}, {2.0, 2.0}}));
  loader->AddRoad(5 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {1.0, 3.0}}));
  loader->AddRoad(6 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 2.0}, {3.0, 3.0}}));
  loader->AddRoad(7 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{2.0, 0.0}, {3.0, 0.0}}));
  loader->AddRoad(8 /* featureId */, true /* oneWay */, 1.0 /* speed */,
                  RoadGeometry::Points({{3.0, 0.0}, {3.0, 1.0}}));

  vector<Joint> const joints = {
      MakeJoint({{0 /* feature id */, 0 /* point id */}}), /* joint at point (0, 0) */
      MakeJoint({{1, 0}, {7, 0}}),                         /* joint at point (2, 0) */
      MakeJoint({{0, 1}, {1, 1}, {2, 0}, {3, 0}}),         /* joint at point (1, 1) */
      MakeJoint({{2, 1}}),                                 /* joint at point (0, 2) */
      MakeJoint({{3, 1}, {4, 1}, {5, 0}, {6, 0}}),         /* joint at point (2, 2) */
      MakeJoint({{4, 0}, {8, 1}}),                         /* joint at point (3, 1) */
      MakeJoint({{5, 1}}),                                 /* joint at point (1, 3) */
      MakeJoint({{6, 1}}),                                 /* joint at point (3, 3) */
      MakeJoint({{7, 1}, {8, 0}}),                         /* joint at point (3, 0) */
  };

  unique_ptr<IndexGraph> graph = make_unique<IndexGraph>(move(loader), estimator);
  graph->Import(joints);
  return graph;
}

// Route through XX graph without any traffic info.
UNIT_TEST(XXGraph_EmptyTrafficColoring)
{
  classificator::Load();
  shared_ptr<EdgeEstimator> estimator =
      EdgeEstimator::CreateForCar(*make_shared<CarModelFactory>()->GetVehicleModel());

  unique_ptr<IndexGraph> graph = BuildXXGraph(estimator);
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {1, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through XX graph with SpeedGroup::G0 on F3.
UNIT_TEST(XXGraph_G0onF3)
{
  classificator::Load();
  shared_ptr<EdgeEstimator> estimator =
      EdgeEstimator::CreateForCar(*make_shared<CarModelFactory>()->GetVehicleModel());
  TrafficInfo::Coloring coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G0}};
  shared_ptr<TrafficInfo> trafficInfo = make_shared<TrafficInfo>();
  trafficInfo->SetColoringForTesting(coloring);
  estimator->SetTrafficInfo(trafficInfo);

  unique_ptr<IndexGraph> graph = BuildXXGraph(estimator);
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}

// Route through XX graph SpeedGroup::G1 on F3 and F6, SpeedGroup::G4 on F8 and F4.
UNIT_TEST(XXGraph_G0onF3andF6andG4onF8andF4)
{
  shared_ptr<EdgeEstimator> estimator =
      EdgeEstimator::CreateForCar(*make_shared<CarModelFactory>()->GetVehicleModel());
  TrafficInfo::Coloring coloring = {
      {{3 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G0},
      {{6 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G0},
      {{8 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G4},
      {{7 /* feature id */, 0 /* segment id */, TrafficInfo::RoadSegmentId::kForwardDirection},
       SpeedGroup::G4}};
  shared_ptr<TrafficInfo> trafficInfo = make_shared<TrafficInfo>();
  trafficInfo->SetColoringForTesting(coloring);
  estimator->SetTrafficInfo(trafficInfo);

  unique_ptr<IndexGraph> graph = BuildXXGraph(estimator);
  IndexGraphStarter starter(*graph, RoadPoint(1, 0) /* start */, RoadPoint(6, 1) /* finish */);
  vector<m2::PointD> const expectedGeom = {{2 /* x */, 0 /* y */}, {3, 0}, {3, 1}, {2, 2}, {3, 3}};
  TestRouteGeometry(starter, AStarAlgorithm<IndexGraphStarter>::Result::OK, expectedGeom);
}
}  // namespace
