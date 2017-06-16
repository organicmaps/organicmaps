#include "routing/index_router.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/bicycle_directions.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/restriction_loader.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/turns_generator.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/distance.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "platform/mwm_traits.hpp"

#include "base/exception.hpp"

#include <algorithm>

using namespace routing;

namespace
{
size_t constexpr kMaxRoadCandidates = 6;
float constexpr kProgressInterval = 2;
uint32_t constexpr kDrawPointsPeriod = 10;

// If user left the route within this range, adjust the route. Else full rebuild.
double constexpr kAdjustRange = 5000.0;
// Propagate astar wave for some distance to try to find a better return point.
double constexpr kAdjustDistance = 2000.0;
// Full rebuild if distance to finish is lesser.
double const kMinDistanceToFinish = kAdjustDistance * 2.0;

template <typename Graph>
IRouter::ResultCode ConvertResult(typename AStarAlgorithm<Graph>::Result result)
{
  switch (result)
  {
  case AStarAlgorithm<Graph>::Result::NoPath: return IRouter::RouteNotFound;
  case AStarAlgorithm<Graph>::Result::Cancelled: return IRouter::Cancelled;
  case AStarAlgorithm<Graph>::Result::OK: return IRouter::NoError;
  }
}

template <typename Graph>
IRouter::ResultCode FindPath(
    typename Graph::TVertexType const & start, typename Graph::TVertexType const & finish,
    RouterDelegate const & delegate, Graph & graph,
    typename AStarAlgorithm<Graph>::TOnVisitedVertexCallback const & onVisitedVertexCallback,
    RoutingResult<typename Graph::TVertexType> & routingResult)
{
  AStarAlgorithm<Graph> algorithm;
  return ConvertResult<Graph>(algorithm.FindPathBidirectional(graph, start, finish, routingResult,
                                                       delegate, onVisitedVertexCallback));
}

bool IsDeadEnd(Segment const & segment, bool isOutgoing, WorldGraph & worldGraph)
{
  size_t constexpr kDeadEndTestLimit = 50;

  auto const getVertexByEdgeFn = [](SegmentEdge const & edge) {
    return edge.GetTarget();
  };

  // Note. If |isOutgoing| == true outgoing edges are looked for.
  // If |isOutgoing| == false it's the finish. So ingoing edges are looked for.
  auto const getOutgoingEdgesFn = [isOutgoing](WorldGraph & graph, Segment const & u,
                                               vector<SegmentEdge> & edges) {
    graph.GetEdgeList(u, isOutgoing, false /* isLeap */, edges);
  };

  return !CheckGraphConnectivity(segment, kDeadEndTestLimit, worldGraph,
                                 getVertexByEdgeFn, getOutgoingEdgesFn);
}

bool AllMwmsHaveRoutingIndex(Index & index, Route & route)
{
  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);

  bool result = true;
  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::COUNTRY)
    {
      if (!version::MwmTraits(info->m_version).HasRoutingIndex())
      {
        result = false;
        route.AddAbsentCountry(info->GetCountryName());
      }
    }
  }

  return result;
}
}  // namespace

namespace routing
{
IndexRouter::IndexRouter(string const & name, TCountryFileFn const & countryFileFn,
                         CourntryRectFn const & countryRectFn, shared_ptr<NumMwmIds> numMwmIds,
                         unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                         shared_ptr<TrafficStash> trafficStash,
                         shared_ptr<VehicleModelFactory> vehicleModelFactory,
                         shared_ptr<EdgeEstimator> estimator,
                         unique_ptr<IDirectionsEngine> directionsEngine, Index & index)
  : m_name(name)
  , m_index(index)
  , m_countryFileFn(countryFileFn)
  , m_countryRectFn(countryRectFn)
  , m_numMwmIds(numMwmIds)
  , m_numMwmTree(move(numMwmTree))
  , m_trafficStash(trafficStash)
  , m_indexManager(countryFileFn, m_index)
  , m_roadGraph(index, IRoadGraph::Mode::ObeyOnewayTag, vehicleModelFactory)
  , m_vehicleModelFactory(vehicleModelFactory)
  , m_estimator(estimator)
  , m_directionsEngine(move(directionsEngine))
{
  CHECK(!m_name.empty(), ());
  CHECK(m_numMwmIds, ());
  CHECK(m_numMwmTree, ());
  CHECK(m_trafficStash, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK(m_estimator, ());
  CHECK(m_directionsEngine, ());
}

IRouter::ResultCode IndexRouter::CalculateRoute(m2::PointD const & startPoint,
                                                m2::PointD const & startDirection,
                                                m2::PointD const & finalPoint, bool adjust,
                                                RouterDelegate const & delegate, Route & route)
{
  if (!AllMwmsHaveRoutingIndex(m_index, route))
    return IRouter::ResultCode::FileTooOld;

  string const startCountry = m_countryFileFn(startPoint);
  string const finishCountry = m_countryFileFn(finalPoint);

  return CalculateRoute(startCountry, finishCountry, false /* blockMwmBorders */, startPoint,
                        startDirection, finalPoint, adjust, delegate, route);
}

IRouter::ResultCode IndexRouter::CalculateRoute(string const & startCountry,
                                                string const & finishCountry, bool forSingleMwm,
                                                m2::PointD const & startPoint,
                                                m2::PointD const & startDirection,
                                                m2::PointD const & finalPoint, bool adjust,
                                                RouterDelegate const & delegate, Route & route)
{
  try
  {
    auto const startFile = platform::CountryFile(startCountry);
    auto const finishFile = platform::CountryFile(finishCountry);

    if (adjust && !m_lastRoute.IsEmpty() && finalPoint == m_lastRoute.GetFinish())
    {
      double const distanceToRoute = m_lastRoute.CalcDistance(startPoint);
      double const distanceToFinish = MercatorBounds::DistanceOnEarth(startPoint, finalPoint);
      if (distanceToRoute <= kAdjustRange && distanceToFinish >= kMinDistanceToFinish)
        return AdjustRoute(startFile, startPoint, startDirection, finalPoint, delegate, route);
    }

    return DoCalculateRoute(startFile, finishFile, forSingleMwm, startPoint, startDirection,
                            finalPoint, delegate, route);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't find path from", MercatorBounds::ToLatLon(startPoint), "to",
                 MercatorBounds::ToLatLon(finalPoint), ":\n ", e.what()));
    return IRouter::InternalError;
  }
}

IRouter::ResultCode IndexRouter::DoCalculateRoute(platform::CountryFile const & startCountry,
                                                  platform::CountryFile const & finishCountry,
                                                  bool forSingleMwm, m2::PointD const & startPoint,
                                                  m2::PointD const & /* startDirection */,
                                                  m2::PointD const & finalPoint,
                                                  RouterDelegate const & delegate, Route & route)
{
  m_lastRoute.Clear();

  TrafficStash::Guard guard(*m_trafficStash);
  WorldGraph graph = MakeWorldGraph();

  bool const isStartMwmLoaded = m_index.IsLoaded(startCountry);
  bool const isFinishMwmLoaded = m_index.IsLoaded(finishCountry);
  if (!isStartMwmLoaded)
    route.AddAbsentCountry(startCountry.GetName());
  if (!isFinishMwmLoaded)
    route.AddAbsentCountry(finishCountry.GetName());
  if (!isStartMwmLoaded || !isFinishMwmLoaded)
    return IRouter::NeedMoreMaps;

  Segment startSegment;
  if (!FindClosestSegment(startCountry, startPoint, true /* isOutgoing */, graph, startSegment))
    return IRouter::StartPointNotFound;

  Segment finishSegment;
  if (!FindClosestSegment(finishCountry, finalPoint, false /* isOutgoing */, graph, finishSegment))
    return IRouter::EndPointNotFound;

  WorldGraph::Mode mode = WorldGraph::Mode::SingleMwm;
  if (forSingleMwm)
    mode = WorldGraph::Mode::SingleMwm;
  else if (AreMwmsNear(startSegment.GetMwmId(), finishSegment.GetMwmId()))
    mode = WorldGraph::Mode::LeapsIfPossible;
  else
    mode = WorldGraph::Mode::LeapsOnly;
  graph.SetMode(mode);

  LOG(LINFO, ("Routing in mode:", graph.GetMode()));

  IndexGraphStarter starter(IndexGraphStarter::FakeVertex(startSegment, startPoint),
                            IndexGraphStarter::FakeVertex(finishSegment, finalPoint), graph);

  AStarProgress progress(0, 100);
  progress.Initialize(starter.GetStartVertex().GetPoint(), starter.GetFinishVertex().GetPoint());

  uint32_t drawPointsStep = 0;
  auto onVisitJunction = [&](Segment const & from, Segment const & to) {
    m2::PointD const & pointFrom = starter.GetPoint(from, true /* front */);
    m2::PointD const & pointTo = starter.GetPoint(to, true /* front */);
    auto const lastValue = progress.GetLastValue();
    auto const newValue = progress.GetProgressForBidirectedAlgo(pointFrom, pointTo);
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);
    if (drawPointsStep % kDrawPointsPeriod == 0)
      delegate.OnPointCheck(pointFrom);
    ++drawPointsStep;
  };

  RoutingResult<Segment> routingResult;
  IRouter::ResultCode const result = FindPath(starter.GetStart(), starter.GetFinish(), delegate,
                                              starter, onVisitJunction, routingResult);
  if (result != IRouter::NoError)
    return result;

  vector<Segment> segments;
  IRouter::ResultCode const leapsResult =
      ProcessLeaps(routingResult.path, delegate, mode, starter, segments);
  if (leapsResult != IRouter::NoError)
    return leapsResult;

  CHECK_GREATER_OR_EQUAL(segments.size(), routingResult.path.size(), ());

  auto redressResult = RedressRoute(segments, delegate, forSingleMwm, starter, route);
  if (redressResult != IRouter::NoError)
    return redressResult;

  m_lastRoute.Init(startPoint, finalPoint);
  for (Segment const & segment : routingResult.path)
    m_lastRoute.AddStep(segment, starter.GetPoint(segment, true /* front */));

  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::AdjustRoute(platform::CountryFile const & startCountry,
                                             m2::PointD const & startPoint,
                                             m2::PointD const & startDirection,
                                             m2::PointD const & finalPoint,
                                             RouterDelegate const & delegate, Route & route)
{
  my::Timer timer;
  TrafficStash::Guard guard(*m_trafficStash);
  WorldGraph graph = MakeWorldGraph();
  graph.SetMode(WorldGraph::Mode::NoLeaps);

  Segment startSegment;
  if (!FindClosestSegment(startCountry, startPoint, true /* isOutgoing */, graph, startSegment))
    return IRouter::StartPointNotFound;

  IndexGraphStarter starter(
      IndexGraphStarter::FakeVertex(startSegment, startPoint),
      IndexGraphStarter::FakeVertex(m_lastRoute.GetFinishSegment(), finalPoint), graph);

  AStarProgress progress(0, 100);
  progress.Initialize(starter.GetStartVertex().GetPoint(), starter.GetFinishVertex().GetPoint());

  uint32_t drawPointsStep = 0;
  auto onVisitJunction = [&](Segment const & from, Segment const & /* to */) {
    m2::PointD const & point = starter.GetPoint(from, true /* front */);
    auto const lastValue = progress.GetLastValue();
    auto const newValue = progress.GetProgressForDirectedAlgo(point);
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);
    if (drawPointsStep % kDrawPointsPeriod == 0)
      delegate.OnPointCheck(point);
    ++drawPointsStep;
  };

  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Segment> routingResult;

  auto const & steps = m_lastRoute.GetSteps();
  set<Segment> routeSegmentsSet;
  for (auto const & step : steps)
  {
    auto const & segment = step.GetSegment();
    if (!IndexGraphStarter::IsFakeSegment(segment))
      routeSegmentsSet.insert(segment);
  }

  double const requiredDistanceToFinish =
      MercatorBounds::DistanceOnEarth(startPoint, finalPoint) - kAdjustDistance;
  CHECK_GREATER(requiredDistanceToFinish, 0.0, ());

  auto const isFinalVertex = [&](Segment const & vertex) {
    if (vertex == starter.GetFinish())
      return true;

    return routeSegmentsSet.count(vertex) > 0 &&
           MercatorBounds::DistanceOnEarth(
               starter.GetPoint(vertex, true /* front */), finalPoint) <= requiredDistanceToFinish;
  };

  auto const code = ConvertResult<IndexGraphStarter>(
      algorithm.FindPath(starter, starter.GetStart(), starter.GetFinish(), routingResult, delegate,
                         onVisitJunction, isFinalVertex));
  if (code != IRouter::NoError)
    return code;

  size_t const adjustingRouteSize = routingResult.path.size();
  AppendRemainingRoute(routingResult.path);

  auto const redressResult = RedressRoute(routingResult.path, delegate, false, starter, route);
  if (redressResult != IRouter::NoError)
    return redressResult;

  LOG(LINFO, ("Adjust route, elapsed:", timer.ElapsedSeconds(), ", start:",
              MercatorBounds::ToLatLon(startPoint), ", finish:",
              MercatorBounds::ToLatLon(finalPoint), ", old route:", steps.size(), ", new route:",
              routingResult.path.size(), ", adjust:", adjustingRouteSize));

  return IRouter::NoError;
}

void IndexRouter::AppendRemainingRoute(vector<Segment> & route) const
{
  auto const steps = m_lastRoute.GetSteps();
  Segment const joinSegment = route.back();

  // Route to finish found, append is not needed.
  if (IndexGraphStarter::IsFakeSegment(joinSegment))
    return;

  for (size_t i = 0; i < steps.size(); ++i)
  {
    if (steps[i].GetSegment() == joinSegment)
    {
      for (size_t j = i + 1; j < steps.size(); ++j)
        route.push_back(steps[j].GetSegment());

      return;
    }
  }

  CHECK(false,
        ("Can't find", joinSegment, ", m_routeSegments:", steps.size(), "path:", route.size()));
}

WorldGraph IndexRouter::MakeWorldGraph()
{
  WorldGraph graph(
      make_unique<CrossMwmGraph>(m_numMwmIds, m_numMwmTree, m_vehicleModelFactory, m_countryRectFn,
                                 m_index, m_indexManager),
      IndexGraphLoader::Create(m_numMwmIds, m_vehicleModelFactory, m_estimator, m_index),
      m_estimator);
  return graph;
}

bool IndexRouter::FindClosestSegment(platform::CountryFile const & file, m2::PointD const & point,
                                     bool isOutgoing, WorldGraph & worldGraph,
                                     Segment & closestSegment) const
{
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  auto const mwmId = MwmSet::MwmId(handle.GetInfo());
  NumMwmId const numMwmId = m_numMwmIds->GetId(file);

  vector<pair<Edge, Junction>> candidates;
  m_roadGraph.FindClosestEdges(point, kMaxRoadCandidates, candidates);

  double minDistance = numeric_limits<double>::max();
  size_t minIndex = candidates.size();

  for (size_t i = 0; i < candidates.size(); ++i)
  {
    Edge const & edge = candidates[i].first;
    Segment const segment(numMwmId, edge.GetFeatureId().m_index, edge.GetSegId(), edge.IsForward());

    if (edge.GetFeatureId().m_mwmId != mwmId || IsDeadEnd(segment, isOutgoing, worldGraph))
      continue;

    m2::DistanceToLineSquare<m2::PointD> squaredDistance;
    squaredDistance.SetBounds(edge.GetStartJunction().GetPoint(), edge.GetEndJunction().GetPoint());
    double const distance = squaredDistance(point);
    if (distance < minDistance)
    {
      minDistance = distance;
      minIndex = i;
    }
  }

  if (minIndex == candidates.size())
    return false;

  Edge const & closestEdge = candidates[minIndex].first;
  closestSegment = Segment(m_numMwmIds->GetId(file), closestEdge.GetFeatureId().m_index,
                           closestEdge.GetSegId(), true /* forward */);
  return true;
}

IRouter::ResultCode IndexRouter::ProcessLeaps(vector<Segment> const & input,
                                              RouterDelegate const & delegate,
                                              WorldGraph::Mode prevMode,
                                              IndexGraphStarter & starter,
                                              vector<Segment> & output)
{
  output.reserve(input.size());

  WorldGraph & worldGraph = starter.GetGraph();
  worldGraph.SetMode(WorldGraph::Mode::NoLeaps);

  for (size_t i = 0; i < input.size(); ++i)
  {
    Segment const & current = input[i];

    if ((prevMode == WorldGraph::Mode::LeapsOnly && IndexGraphStarter::IsFakeSegment(current))
      || (prevMode != WorldGraph::Mode::LeapsOnly && !starter.IsLeap(current.GetMwmId())))
    {
      output.push_back(current);
      continue;
    }

    // Clear previous loaded graphs to not spend too much memory at one time.
    worldGraph.ClearIndexGraphs();

    ++i;
    CHECK_LESS(i, input.size(), ());
    Segment const & next = input[i];

    CHECK(!IndexGraphStarter::IsFakeSegment(current), ());
    CHECK(!IndexGraphStarter::IsFakeSegment(next), ());
    CHECK_EQUAL(
      current.GetMwmId(), next.GetMwmId(),
      ("Different mwm ids for leap enter and exit, i:", i, "size of input:", input.size()));

    IRouter::ResultCode result = IRouter::InternalError;
    RoutingResult<Segment> routingResult;
    // In case of leaps from the start to its mwm transition and from finish mwm transition
    // route calculation should be made on the world graph (WorldGraph::Mode::NoLeaps).
    if ((current.GetMwmId() == starter.GetStartVertex().GetMwmId()
        || current.GetMwmId() == starter.GetFinishVertex().GetMwmId())
        && prevMode == WorldGraph::Mode::LeapsOnly)
    {
      // World graph route.
      result = FindPath(current, next, delegate, worldGraph, {} /* onVisitedVertexCallback */, routingResult);
    }
    else
    {
      // Single mwm route.
      IndexGraph & indexGraph = worldGraph.GetIndexGraph(current.GetMwmId());
      result = FindPath(current, next, delegate, indexGraph, {} /* onVisitedVertexCallback */, routingResult);
    }
    if (result != IRouter::NoError)
      return result;
    output.insert(output.end(), routingResult.path.cbegin(), routingResult.path.cend());
  }

  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::RedressRoute(vector<Segment> const & segments,
                                              RouterDelegate const & delegate, bool forSingleMwm,
                                              IndexGraphStarter & starter, Route & route) const
{
  vector<Junction> junctions;
  size_t const numPoints = IndexGraphStarter::GetRouteNumPoints(segments);
  junctions.reserve(numPoints);

  for (size_t i = 0; i < numPoints; ++i)
  {
    // TODO: Use real altitudes for pedestrian and bicycle routing.
    junctions.emplace_back(starter.GetRoutePoint(segments, i), feature::kDefaultAltitudeMeters);
  }

  IndexRoadGraph roadGraph(m_numMwmIds, starter, segments, junctions, m_index);
  if (!forSingleMwm)
    starter.GetGraph().SetMode(WorldGraph::Mode::NoLeaps);

  CHECK(m_directionsEngine, ());
  ReconstructRoute(*m_directionsEngine, roadGraph, m_trafficStash, delegate,
                   false /* hasAltitude */, junctions, route);

  if (!route.IsValid())
  {
    LOG(LERROR, ("ReconstructRoute failed. Segments:", segments.size()));
    return IRouter::InternalError;
  }

  Route::TTimes times;
  times.reserve(segments.size());
  double time = 0.0;
  times.emplace_back(static_cast<uint32_t>(0), 0.0);
  // First and last segments are fakes: skip it.
  for (size_t i = 1; i < segments.size() - 1; ++i)
  {
    times.emplace_back(static_cast<uint32_t>(i), time);
    time += starter.CalcSegmentWeight(segments[i]);
  }

  route.SetSectionTimes(move(times));

  if (delegate.IsCancelled())
    return IRouter::Cancelled;

  return IRouter::NoError;
}

bool IndexRouter::AreMwmsNear(NumMwmId startId, NumMwmId finishId) const
{
  m2::RectD const startMwmRect = m_countryRectFn(m_numMwmIds->GetFile(startId).GetName());
  bool areMwmsNear = false;
  m_numMwmTree->ForEachInRect(startMwmRect, [&](NumMwmId id) {
    if (id == finishId)
      areMwmsNear = true;
  });
  return areMwmsNear;
}

// static
unique_ptr<IndexRouter> IndexRouter::CreateCarRouter(TCountryFileFn const & countryFileFn,
                                                     CourntryRectFn const & coutryRectFn,
                                                     shared_ptr<NumMwmIds> numMwmIds,
                                                     unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                                                     traffic::TrafficCache const & trafficCache,
                                                     Index & index)
{
  CHECK(numMwmIds, ());
  auto vehicleModelFactory = make_shared<CarModelFactory>();
  // @TODO Bicycle turn generation engine is used now. It's ok for the time being.
  // But later a special car turn generation engine should be implemented.
  auto directionsEngine = make_unique<BicycleDirectionsEngine>(index, numMwmIds);

  double maxSpeed = 0.0;
  numMwmIds->ForEachId([&](NumMwmId id) {
    string const & country = numMwmIds->GetFile(id).GetName();
    double const mwmMaxSpeed =
        vehicleModelFactory->GetVehicleModelForCountry(country)->GetMaxSpeed();
    maxSpeed = max(maxSpeed, mwmMaxSpeed);
  });

  auto trafficStash = make_shared<TrafficStash>(trafficCache, numMwmIds);

  auto estimator = EdgeEstimator::CreateForCar(trafficStash, maxSpeed);
  auto router = make_unique<IndexRouter>(
      "astar-bidirectional-car", countryFileFn, coutryRectFn, numMwmIds, move(numMwmTree),
      trafficStash, vehicleModelFactory, estimator, move(directionsEngine), index);
  return router;
}
}  // namespace routing
