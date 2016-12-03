#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "routing/car_model.hpp"

namespace routing_test
{
using namespace routing;

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

shared_ptr<EdgeEstimator> CreateEstimator(TrafficInfoGetterNoJam const & trafficGetter)
{
  return EdgeEstimator::CreateForCar(*make_shared<CarModelFactory>()->GetVehicleModel(), trafficGetter);
}

AStarAlgorithm<IndexGraphStarter>::Result CalculateRoute(IndexGraphStarter & starter,
                                                         vector<RoadPoint> & roadPoints)
{
  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Joint::Id> routingResult;
  auto const resultCode = algorithm.FindPath(
      starter, starter.GetStartJoint(), starter.GetFinishJoint(), routingResult, {}, {});

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
    // When PR with applying restricions is merged IndexGraph::GetRoad() should be used here instead.
    RoadGeometry roadGeom = starter.GetGraph().GetGeometry().GetRoad(route[i].GetFeatureId());
    CHECK_LESS(route[i].GetPointId(), roadGeom.GetPointsCount(), ());
    TEST_EQUAL(expectedRouteGeom[i], roadGeom.GetPoint(route[i].GetPointId()), ());
  }
}
}  // namespace routing_test
