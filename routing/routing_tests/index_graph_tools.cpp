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

shared_ptr<EdgeEstimator> CreateEstimator(traffic::TrafficCache const & trafficCache)
{
  return EdgeEstimator::CreateForCar(*make_shared<CarModelFactory>()->GetVehicleModel(),
                                     trafficCache);
}

AStarAlgorithm<IndexGraphStarter>::Result CalculateRoute(IndexGraphStarter & starter,
                                                          vector<Segment> & roadPoints)
{
  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Segment> routingResult;
  auto const resultCode = algorithm.FindPathBidirectional(
      starter, starter.GetStart(), starter.GetFinish(), routingResult, {}, {});

  roadPoints = routingResult.path;
  return resultCode;
}

void TestRouteGeometry(IndexGraphStarter & starter,
                       AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                       vector<m2::PointD> const & expectedRouteGeom)
{
  vector<Segment> route;
  auto const resultCode = CalculateRoute(starter, route);
  TEST_EQUAL(resultCode, expectedRouteResult, ());
  TEST_EQUAL(IndexGraphStarter::GetRouteNumPoints(route), expectedRouteGeom.size(), ());

  for (size_t i = 0; i < IndexGraphStarter::GetRouteNumPoints(route); ++i)
    TEST_EQUAL(expectedRouteGeom[i], starter.GetRoutePoint(route, i), ());
}
}  // namespace routing_test
