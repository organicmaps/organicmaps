#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "routing_common/car_model.hpp"

#include "base/assert.hpp"
#include "base/scope_guard.hpp"

namespace routing_test
{
using namespace routing;

// TestGeometryLoader ------------------------------------------------------------------------------
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

// TestIndexGraphLoader ----------------------------------------------------------------------------
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

// TestIndexGraphTopology --------------------------------------------------------------------------
TestIndexGraphTopology::TestIndexGraphTopology(uint32_t numVertices) : m_numVertices(numVertices) {}

void TestIndexGraphTopology::AddDirectedEdge(uint32_t vertexId1, uint32_t vertexId2, double weight)
{
  m_edgeRequests.emplace_back(vertexId1, vertexId2, weight);
}

bool TestIndexGraphTopology::FindPath(uint32_t start, uint32_t finish, double & pathWeight,
                                      vector<pair<uint32_t, uint32_t>> & pathEdges)
{
  // Edges of the index graph are segments, so we need a loop at finish
  // for the end of our path and another loop at start for the bidirectional search.
  AddDirectedEdge(start, start, 0.0);
  AddDirectedEdge(finish, finish, 0.0);
  MY_SCOPE_GUARD(cleanup, [&]() {
    m_edgeRequests.pop_back();
    m_edgeRequests.pop_back();
  });

  BuildGraphFromRequests();
  auto const worldGraph = PrepareIndexGraph();
  CHECK(worldGraph != nullptr, ());

  if (m_joints[start].GetSize() == 0 || m_joints[finish].GetSize() == 0)
  {
    pathWeight = 0;
    pathEdges.clear();
    return false;
  }

  auto const startRoadPoint = m_joints[start].GetEntry(0);
  auto const finishRoadPoint = m_joints[finish].GetEntry(0);

  auto const fakeStart =
      IndexGraphStarter::FakeVertex(kTestNumMwmId, startRoadPoint.GetFeatureId(),
                                    startRoadPoint.GetPointId(), m2::PointD::Zero());
  auto const fakeFinish =
      IndexGraphStarter::FakeVertex(kTestNumMwmId, finishRoadPoint.GetFeatureId(),
                                    finishRoadPoint.GetPointId(), m2::PointD::Zero());

  IndexGraphStarter starter(fakeStart, fakeFinish, *worldGraph);

  vector<Segment> routeSegs;
  double timeSec;
  auto const resultCode = CalculateRoute(starter, routeSegs, timeSec);

  if (resultCode == AStarAlgorithm<IndexGraphStarter>::Result::NoPath)
    return false;
  CHECK_EQUAL(resultCode, AStarAlgorithm<IndexGraphStarter>::Result::OK, ());

  CHECK_GREATER_OR_EQUAL(routeSegs.size(), 2, ());
  CHECK_EQUAL(routeSegs.front(), starter.GetStart(), ());
  CHECK_EQUAL(routeSegs.back(), starter.GetFinish(), ());
  // We are not interested in fake start and finish.
  pathEdges.resize(routeSegs.size() - 2);
  pathWeight = 0;
  for (size_t i = 1; i + 1 < routeSegs.size(); ++i)
  {
    auto const & seg = routeSegs[i];
    auto const it = m_segmentToEdge.find(seg);
    CHECK(it != m_segmentToEdge.cend(), ());
    auto const & edge = it->second;
    pathEdges[i - 1] = edge;
    pathWeight += m_segmentWeights[seg];
  }

  CHECK(!pathEdges.empty(), ());
  // The loop from finish to finish.
  pathEdges.pop_back();

  return true;
}

unique_ptr<WorldGraph> TestIndexGraphTopology::PrepareIndexGraph()
{
  unique_ptr<TestGeometryLoader> loader = make_unique<TestGeometryLoader>();

  for (auto const & p : m_segmentWeights)
  {
    double const weight = p.second;
    loader->AddRoad(p.first.GetFeatureId(), true /* oneWay */, 1.0 /* speed */,
                    RoadGeometry::Points({{0.0, 0.0}, {weight, 0.0}}));
  }

  BuildJoints();

  shared_ptr<EdgeEstimator> estimator =
      EdgeEstimator::CreateForWeightedDirectedGraph(m_segmentWeights);
  return BuildWorldGraph(move(loader), estimator, m_joints);
}

void TestIndexGraphTopology::ClearGraphInternal()
{
  m_edgeWeights.clear();
  m_segmentWeights.clear();
  m_segmentToEdge.clear();
  m_outgoingSegments.clear();
  m_incomingSegments.clear();
  m_joints.clear();
}

void TestIndexGraphTopology::BuildJoints()
{
  m_joints.resize(m_numVertices);
  for (uint32_t i = 0; i < m_numVertices; ++i)
  {
    auto & joint = m_joints[i];

    for (auto const & segment : m_outgoingSegments[i])
      joint.AddPoint(RoadPoint(segment.GetFeatureId(), segment.GetPointId(false /* front */)));

    for (auto const & segment : m_incomingSegments[i])
      joint.AddPoint(RoadPoint(segment.GetFeatureId(), segment.GetPointId(true /* front */)));
  }
}

void TestIndexGraphTopology::BuildGraphFromRequests()
{
  ClearGraphInternal();
  for (size_t i = 0; i < m_edgeRequests.size(); ++i)
  {
    auto const & req = m_edgeRequests[i];
    BuildSegmentFromEdge(static_cast<uint32_t>(i), req.m_src, req.m_dst, req.m_weight);
  }
}

void TestIndexGraphTopology::BuildSegmentFromEdge(uint32_t edgeId, uint32_t vertexId1,
                                                  uint32_t vertexId2, double weight)
{
  auto const edge = make_pair(vertexId1, vertexId2);
  auto p = m_edgeWeights.emplace(edge, weight);
  CHECK(p.second, ("Multi-edges are not allowed"));

  uint32_t const featureId = edgeId;
  Segment segment(kTestNumMwmId, featureId, 0 /* segmentIdx */, true /* forward */);

  m_segmentWeights[segment] = weight;
  m_segmentToEdge[segment] = edge;
  m_outgoingSegments[vertexId1].push_back(segment);
  m_incomingSegments[vertexId2].push_back(segment);
}

// Functions ---------------------------------------------------------------------------------------
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

shared_ptr<EdgeEstimator> CreateEstimatorForCar(traffic::TrafficCache const & trafficCache)
{
  auto numMwmIds = make_shared<NumMwmIds>();
  auto stash = make_shared<TrafficStash>(trafficCache, numMwmIds);
  return CreateEstimatorForCar(stash);
}

shared_ptr<EdgeEstimator> CreateEstimatorForCar(shared_ptr<TrafficStash> trafficStash)
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
      starter, starter.GetStart(), starter.GetFinish(), routingResult, {} /* cancellable */,
      {} /* onVisitedVertexCallback */);

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
      expectedRouteGeom.empty())
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
