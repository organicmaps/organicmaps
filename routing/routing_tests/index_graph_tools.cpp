#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "routing_common/car_model.hpp"

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

IndexGraph & TestIndexGraphLoader::GetIndexGraph(NumMwmId mwmId)
{
  auto it = m_graphs.find(mwmId);
  CHECK(it != m_graphs.end(), ("Not found mwm", mwmId));
  return *it->second;
}

void TestIndexGraphLoader::Clear() { m_graphs.clear(); }

void TestIndexGraphLoader::AddGraph(NumMwmId mwmId, unique_ptr<IndexGraph> graph)
{
  auto it = m_graphs.find(mwmId);
  CHECK(it == m_graphs.end(), ("Already contains mwm", mwmId));
  m_graphs[mwmId] = move(graph);
}

unique_ptr<WorldGraph> BuildWorldGraph(unique_ptr<TestGeometryLoader> geometryLoader,
                                       shared_ptr<EdgeEstimator> estimator,
                                       vector<Joint> const & joints)
{
  auto graph = make_unique<IndexGraph>(move(geometryLoader), estimator);
  graph->Import(joints);
  auto indexLoader = make_unique<TestIndexGraphLoader>();
  indexLoader->AddGraph(kTestNumMwmId, move(graph));
  return make_unique<WorldGraph>(nullptr /* crossMwmGraph */, move(indexLoader), estimator);
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
  auto numMwmIds = make_shared<NumMwmIds>();
  auto stash = make_shared<TrafficStash>(trafficCache, numMwmIds);
  return CreateEstimator(stash);
}

shared_ptr<EdgeEstimator> CreateEstimator(shared_ptr<TrafficStash> trafficStash)
{
  return EdgeEstimator::CreateForCar(trafficStash, 90.0 /* maxSpeedKMpH */);
}

AStarAlgorithm<IndexGraphStarter>::Result CalculateRoute(IndexGraphStarter & starter,
                                                         vector<Segment> & roadPoints,
                                                         double & timeSec)
{
  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Segment> routingResult;
  auto const resultCode = algorithm.FindPathBidirectional(
      starter, starter.GetStart(), starter.GetFinish(), routingResult, {}, {});

  timeSec = routingResult.distance;

  roadPoints = routingResult.path;
  return resultCode;
}

void TestRouteGeometry(IndexGraphStarter & starter,
                       AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                       vector<m2::PointD> const & expectedRouteGeom)
{
  vector<Segment> routeSegs;
  double timeSec = 0.0;
  auto const resultCode = CalculateRoute(starter, routeSegs, timeSec);

  if (AStarAlgorithm<IndexGraphStarter>::Result::NoPath == expectedRouteResult &&
      expectedRouteGeom == vector<m2::PointD>())
  {
    // The route goes through a restriction. So there's no choice for building route
    // except for going through restriction. So no path.
    return;
  }

  TEST_EQUAL(resultCode, expectedRouteResult, ());
  if (resultCode != AStarAlgorithm<IndexGraphStarter>::Result::OK)
    return;

  CHECK(!routeSegs.empty(), ());
  vector<m2::PointD> geom;

  auto const pushPoint = [&geom](m2::PointD const & point) {
    if (geom.empty() || geom.back() != point)
      geom.push_back(point);
  };

  for (size_t i = 0; i < routeSegs.size(); ++i)
  {
    m2::PointD const & pnt = starter.GetPoint(routeSegs[i], false /* front */);
    // Note. In case of A* router all internal points of route are duplicated.
    // So it's necessary to exclude the duplicates.
    pushPoint(pnt);
  }

  pushPoint(starter.GetPoint(routeSegs.back(), false /* front */));
  TEST_EQUAL(geom, expectedRouteGeom, ());
}

void TestRouteTime(IndexGraphStarter & starter,
                   AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                   double expectedTime)
{
  vector<Segment> routeSegs;
  double timeSec = 0.0;
  auto const resultCode = CalculateRoute(starter, routeSegs, timeSec);

  TEST_EQUAL(resultCode, expectedRouteResult, ());
  double const kEpsilon = 1e-5;
  TEST(my::AlmostEqualAbs(timeSec, expectedTime, kEpsilon), ());
}

void TestRestrictions(vector<m2::PointD> const & expectedRouteGeom,
                      AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                      routing::IndexGraphStarter::FakeVertex const & start,
                      routing::IndexGraphStarter::FakeVertex const & finish,
                      RestrictionVec && restrictions, RestrictionTest & restrictionTest)
{
  restrictionTest.SetRestrictions(move(restrictions));
  restrictionTest.SetStarter(start, finish);
  TestRouteGeometry(*restrictionTest.m_starter, expectedRouteResult, expectedRouteGeom);
}
}  // namespace routing_test
