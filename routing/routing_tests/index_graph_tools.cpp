#include "routing/routing_tests/index_graph_tools.hpp"

#include "testing/testing.hpp"

#include "routing/base/routing_result.hpp"

#include "routing_common/car_model.hpp"

#include "base/assert.hpp"

namespace routing_test
{
using namespace routing;

namespace
{
double constexpr kEpsilon = 1e-6;

template <typename Graph>
Graph & GetGraph(unordered_map<NumMwmId, unique_ptr<Graph>> const & graphs, NumMwmId mwmId)
{
  auto it = graphs.find(mwmId);
  CHECK(it != graphs.end(), ("Not found graph for mwm", mwmId));
  return *it->second;
}

template <typename Graph>
void AddGraph(unordered_map<NumMwmId, unique_ptr<Graph>> & graphs, NumMwmId mwmId,
              unique_ptr<Graph> graph)
{
  auto it = graphs.find(mwmId);
  CHECK(it == graphs.end(), ("Already contains graph for mwm", mwmId));
  graphs[mwmId] = move(graph);
}
}  // namespace

// RestrictionTest
void RestrictionTest::SetStarter(FakeEnding const & start, FakeEnding const & finish)
{
  CHECK(m_graph != nullptr, ("Init() was not called."));
  m_starter = MakeStarter(start, finish, *m_graph);
}

// TestGeometryLoader ------------------------------------------------------------------------------
void TestGeometryLoader::Load(uint32_t featureId, RoadGeometry & road)
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
  m_roads[featureId].SetPassThroughAllowedForTests(true);
}

void TestGeometryLoader::SetPassThroughAllowed(uint32_t featureId, bool passThroughAllowed)
{
  auto it = m_roads.find(featureId);
  CHECK(it != m_roads.end(), ("No feature", featureId));
  m_roads[featureId].SetPassThroughAllowedForTests(passThroughAllowed);
}

// ZeroGeometryLoader ------------------------------------------------------------------------------
void ZeroGeometryLoader::Load(uint32_t /* featureId */, routing::RoadGeometry & road)
{
  // Any valid road will do.
  auto const points = routing::RoadGeometry::Points({{0.0, 0.0}, {0.0, 1.0}});
  road = RoadGeometry(true /* oneWay */, 1.0 /* speed */, points);
}

// TestIndexGraphLoader ----------------------------------------------------------------------------
IndexGraph & TestIndexGraphLoader::GetIndexGraph(NumMwmId mwmId)
{
  return GetGraph(m_graphs, mwmId);
}

void TestIndexGraphLoader::Clear() { m_graphs.clear(); }

void TestIndexGraphLoader::AddGraph(NumMwmId mwmId, unique_ptr<IndexGraph> graph)
{
  routing_test::AddGraph(m_graphs, mwmId, move(graph));
}

// TestTransitGraphLoader ----------------------------------------------------------------------------
TransitGraph & TestTransitGraphLoader::GetTransitGraph(NumMwmId mwmId, IndexGraph &)
{
  return GetGraph(m_graphs, mwmId);
}

void TestTransitGraphLoader::Clear() { m_graphs.clear(); }

void TestTransitGraphLoader::AddGraph(NumMwmId mwmId, unique_ptr<TransitGraph> graph)
{
  routing_test::AddGraph(m_graphs, mwmId, move(graph));
}

// WeightedEdgeEstimator --------------------------------------------------------------
double WeightedEdgeEstimator::CalcSegmentWeight(Segment const & segment,
                                                RoadGeometry const & /* road */) const
{
  auto const it = m_segmentWeights.find(segment);
  CHECK(it != m_segmentWeights.cend(), ());
  return it->second;
}

double WeightedEdgeEstimator::GetUTurnPenalty() const { return 0.0; }

bool WeightedEdgeEstimator::LeapIsAllowed(NumMwmId /* mwmId */) const { return false; }

// TestIndexGraphTopology --------------------------------------------------------------------------
TestIndexGraphTopology::TestIndexGraphTopology(uint32_t numVertices) : m_numVertices(numVertices) {}

void TestIndexGraphTopology::AddDirectedEdge(Vertex from, Vertex to, double weight)
{
  AddDirectedEdge(m_edgeRequests, from, to, weight);
}

void TestIndexGraphTopology::BlockEdge(Vertex from, Vertex to)
{
  for (auto & r : m_edgeRequests)
  {
    if (r.m_from == from && r.m_to == to)
    {
      r.m_isBlocked = true;
      return;
    }
  }
  CHECK(false, ("Cannot block edge that is not in the graph", from, to));
}

bool TestIndexGraphTopology::FindPath(Vertex start, Vertex finish, double & pathWeight,
                                      vector<Edge> & pathEdges) const
{
  CHECK_LESS(start, m_numVertices, ());
  CHECK_LESS(finish, m_numVertices, ());

  if (start == finish)
  {
    pathWeight = 0.0;
    pathEdges.clear();
    return true;
  }

  auto edgeRequests = m_edgeRequests;
  // Edges of the index graph are segments, so we need a loop at finish
  // for the end of our path and another loop at start for the bidirectional search.
  auto const startFeatureId = static_cast<uint32_t>(edgeRequests.size());
  AddDirectedEdge(edgeRequests, start, start, 0.0);
  // |startSegment| corresponds to edge from |start| to |start| which has featureId |startFeatureId|
  // and the only segment with segmentIdx |0|. It is a loop so direction does not matter.
  auto const startSegment = Segment(kTestNumMwmId, startFeatureId, 0 /* segmentIdx */,
                                    true /* forward */);

  auto const finishFeatureId = static_cast<uint32_t>(edgeRequests.size());
  AddDirectedEdge(edgeRequests, finish, finish, 0.0);
  // |finishSegment| corresponds to edge from |finish| to |finish| which has featureId |finishFeatureId|
  // and the only segment with segmentIdx |0|. It is a loop so direction does not matter.
  auto const finishSegment = Segment(kTestNumMwmId, finishFeatureId, 0 /* segmentIdx */,
                                     true /* forward */);

  Builder builder(m_numVertices);
  builder.BuildGraphFromRequests(edgeRequests);
  auto const worldGraph = builder.PrepareIndexGraph();
  CHECK(worldGraph != nullptr, ());

  AStarAlgorithm<WorldGraph> algorithm;

  RoutingResult<Segment, RouteWeight> routingResult;
  auto const resultCode = algorithm.FindPathBidirectional(
      *worldGraph, startSegment, finishSegment, routingResult, {} /* cancellable */,
      {} /* onVisitedVertexCallback */, {} /* checkLengthCallback */);

  // Check unidirectional AStar returns same result.
  {
    RoutingResult<Segment, RouteWeight> unidirectionalRoutingResult;
    auto const unidirectionalResultCode = algorithm.FindPath(
        *worldGraph, startSegment, finishSegment, unidirectionalRoutingResult, {} /* cancellable */,
        {} /* onVisitedVertexCallback */, {} /* checkLengthCallback */);

    CHECK_EQUAL(resultCode, unidirectionalResultCode, ());
    CHECK(routingResult.m_distance.IsAlmostEqualForTests(unidirectionalRoutingResult.m_distance,
                                                         kEpsilon),
          ("Distances differ:", routingResult.m_distance, unidirectionalRoutingResult.m_distance));
  }

  if (resultCode == AStarAlgorithm<WorldGraph>::Result::NoPath)
    return false;
  CHECK_EQUAL(resultCode, AStarAlgorithm<WorldGraph>::Result::OK, ());

  CHECK_GREATER_OR_EQUAL(routingResult.m_path.size(), 2, ());
  CHECK_EQUAL(routingResult.m_path.front(), startSegment, ());
  CHECK_EQUAL(routingResult.m_path.back(), finishSegment, ());

  pathEdges.reserve(routingResult.m_path.size());
  pathWeight = 0.0;
  for (auto const & s : routingResult.m_path)
  {
    auto const it = builder.m_segmentToEdge.find(s);
    CHECK(it != builder.m_segmentToEdge.cend(), ());
    auto const & edge = it->second;
    pathEdges.push_back(edge);
    pathWeight += builder.m_segmentWeights[s];
  }

  // The loops from start to start and from finish to finish.
  CHECK_GREATER_OR_EQUAL(pathEdges.size(), 2, ());
  CHECK_EQUAL(pathEdges.front().first, pathEdges.front().second, ());
  CHECK_EQUAL(pathEdges.back().first, pathEdges.back().second, ());
  pathEdges.erase(pathEdges.begin());
  pathEdges.pop_back();

  return true;
}

void TestIndexGraphTopology::AddDirectedEdge(vector<EdgeRequest> & edgeRequests, Vertex from,
                                             Vertex to, double weight) const
{
  uint32_t const id = static_cast<uint32_t>(edgeRequests.size());
  edgeRequests.emplace_back(id, from, to, weight);
}

// TestIndexGraphTopology::Builder -----------------------------------------------------------------
unique_ptr<SingleVehicleWorldGraph> TestIndexGraphTopology::Builder::PrepareIndexGraph()
{
  auto loader = make_unique<ZeroGeometryLoader>();
  auto estimator = make_shared<WeightedEdgeEstimator>(m_segmentWeights);

  BuildJoints();

  auto worldGraph = BuildWorldGraph(move(loader), estimator, m_joints);
  worldGraph->GetIndexGraphForTests(kTestNumMwmId).SetRoadAccess(move(m_roadAccess));
  return worldGraph;
}

void TestIndexGraphTopology::Builder::BuildJoints()
{
  m_joints.resize(m_numVertices);
  for (uint32_t i = 0; i < m_joints.size(); ++i)
  {
    auto & joint = m_joints[i];

    for (auto const & segment : m_outgoingSegments[i])
      joint.AddPoint(RoadPoint(segment.GetFeatureId(), segment.GetPointId(false /* front */)));

    for (auto const & segment : m_ingoingSegments[i])
      joint.AddPoint(RoadPoint(segment.GetFeatureId(), segment.GetPointId(true /* front */)));
  }
}

void TestIndexGraphTopology::Builder::BuildGraphFromRequests(vector<EdgeRequest> const & requests)
{
  vector<uint32_t> blockedFeatureIds;
  for (auto const & request : requests)
  {
    BuildSegmentFromEdge(request);
    if (request.m_isBlocked)
      blockedFeatureIds.push_back(request.m_id);
  }

  map<Segment, RoadAccess::Type> segmentTypes;
  for (auto const fid : blockedFeatureIds)
  {
    segmentTypes[Segment(kFakeNumMwmId, fid, 0 /* wildcard segmentIdx */, true)] =
        RoadAccess::Type::No;
  }

  m_roadAccess.SetSegmentTypes(move(segmentTypes));
}

void TestIndexGraphTopology::Builder::BuildSegmentFromEdge(EdgeRequest const & request)
{
  auto const edge = make_pair(request.m_from, request.m_to);
  auto p = m_edgeWeights.emplace(edge, request.m_weight);
  CHECK(p.second, ("Multi-edges are not allowed"));

  uint32_t const featureId = request.m_id;
  Segment const segment(kTestNumMwmId, featureId, 0 /* segmentIdx */, true /* forward */);

  m_segmentWeights[segment] = request.m_weight;
  m_segmentToEdge[segment] = edge;
  m_outgoingSegments[request.m_from].push_back(segment);
  m_ingoingSegments[request.m_to].push_back(segment);
}

// Functions ---------------------------------------------------------------------------------------
unique_ptr<SingleVehicleWorldGraph> BuildWorldGraph(unique_ptr<TestGeometryLoader> geometryLoader,
                                                    shared_ptr<EdgeEstimator> estimator,
                                                    vector<Joint> const & joints)
{
  auto graph = make_unique<IndexGraph>(move(geometryLoader), estimator);
  graph->Import(joints);
  auto indexLoader = make_unique<TestIndexGraphLoader>();
  indexLoader->AddGraph(kTestNumMwmId, move(graph));
  return make_unique<SingleVehicleWorldGraph>(nullptr /* crossMwmGraph */, move(indexLoader),
                                              estimator);
}

unique_ptr<SingleVehicleWorldGraph> BuildWorldGraph(unique_ptr<ZeroGeometryLoader> geometryLoader,
                                                    shared_ptr<EdgeEstimator> estimator,
                                                    vector<Joint> const & joints)
{
  auto graph = make_unique<IndexGraph>(move(geometryLoader), estimator);
  graph->Import(joints);
  auto indexLoader = make_unique<TestIndexGraphLoader>();
  indexLoader->AddGraph(kTestNumMwmId, move(graph));
  return make_unique<SingleVehicleWorldGraph>(nullptr /* crossMwmGraph */, move(indexLoader),
                                              estimator);
}

unique_ptr<TransitWorldGraph> BuildWorldGraph(unique_ptr<TestGeometryLoader> geometryLoader,
                                              shared_ptr<EdgeEstimator> estimator,
                                              vector<Joint> const & joints,
                                              TransitGraph::TransitData const & transitData)
{
  auto indexGraph = make_unique<IndexGraph>(move(geometryLoader), estimator);
  indexGraph->Import(joints);

  auto transitGraph = make_unique<TransitGraph>(kTestNumMwmId, estimator);
  TransitGraph::GateEndings gateEndings;
  MakeGateEndings(transitData.m_gates, kTestNumMwmId, *indexGraph, gateEndings);
  transitGraph->Fill(transitData, gateEndings);

  auto indexLoader = make_unique<TestIndexGraphLoader>();
  indexLoader->AddGraph(kTestNumMwmId, move(indexGraph));

  auto transitLoader = make_unique<TestTransitGraphLoader>();
  transitLoader->AddGraph(kTestNumMwmId, move(transitGraph));
  
  return make_unique<TransitWorldGraph>(nullptr /* crossMwmGraph */, move(indexLoader),
                                        move(transitLoader), estimator);
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
  auto const carModel = CarModelFactory({}).GetVehicleModel();
  return EdgeEstimator::Create(VehicleType::Car, *carModel, trafficStash);
}

AStarAlgorithm<IndexGraphStarter>::Result CalculateRoute(IndexGraphStarter & starter,
                                                         vector<Segment> & roadPoints,
                                                         double & timeSec)
{
  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Segment, RouteWeight> routingResult;

  auto resultCode = algorithm.FindPathBidirectional(
      starter, starter.GetStartSegment(), starter.GetFinishSegment(), routingResult,
      {} /* cancellable */, {} /* onVisitedVertexCallback */,
      [&](RouteWeight const & weight) { return starter.CheckLength(weight); });

  timeSec = routingResult.m_distance.GetWeight();
  roadPoints = routingResult.m_path;
  return resultCode;
}

void TestRouteGeometry(IndexGraphStarter & starter,
                       AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                       vector<m2::PointD> const & expectedRouteGeom)
{
  vector<Segment> routeSegs;
  double timeSec = 0.0;
  auto const resultCode = CalculateRoute(starter, routeSegs, timeSec);

  TEST_EQUAL(resultCode, expectedRouteResult, ());

  if (AStarAlgorithm<IndexGraphStarter>::Result::NoPath == expectedRouteResult &&
      expectedRouteGeom.empty())
  {
    // The route goes through a restriction. So there's no choice for building route
    // except for going through restriction. So no path.
    return;
  }

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

void TestRestrictions(vector<m2::PointD> const & expectedRouteGeom,
                      AStarAlgorithm<IndexGraphStarter>::Result expectedRouteResult,
                      FakeEnding const & start, FakeEnding const & finish,
                      RestrictionVec && restrictions, RestrictionTest & restrictionTest)
{
  restrictionTest.SetRestrictions(move(restrictions));
  restrictionTest.SetStarter(start, finish);
  TestRouteGeometry(*restrictionTest.m_starter, expectedRouteResult, expectedRouteGeom);
}

void TestTopologyGraph(TestIndexGraphTopology const & graph, TestIndexGraphTopology::Vertex from,
                       TestIndexGraphTopology::Vertex to, bool expectedPathFound,
                       double const expectedWeight,
                       vector<TestIndexGraphTopology::Edge> const & expectedEdges)
{
  double pathWeight = 0.0;
  vector<TestIndexGraphTopology::Edge> pathEdges;
  bool const pathFound = graph.FindPath(from, to, pathWeight, pathEdges);
  TEST_EQUAL(pathFound, expectedPathFound, ());
  if (!pathFound)
    return;

  TEST(my::AlmostEqualAbs(pathWeight, expectedWeight, kEpsilon),
       (pathWeight, expectedWeight, pathEdges));
  TEST_EQUAL(pathEdges, expectedEdges, ());
}

FakeEnding MakeFakeEnding(uint32_t featureId, uint32_t segmentIdx, m2::PointD const & point,
                          WorldGraph & graph)
{
  return MakeFakeEnding(Segment(kTestNumMwmId, featureId, segmentIdx, true /* forward */), point,
                        graph);
}
unique_ptr<IndexGraphStarter> MakeStarter(FakeEnding const & start, FakeEnding const & finish,
                                          WorldGraph & graph)
{
  return make_unique<IndexGraphStarter>(start, finish, 0 /* fakeNumerationStart */,
                                        false /* strictForward */, graph);
}
}  // namespace routing_test
