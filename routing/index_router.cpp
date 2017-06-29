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
#include <map>
#include <utility>

using namespace routing;
using namespace std;

namespace
{
size_t constexpr kMaxRoadCandidates = 6;
float constexpr kProgressInterval = 2;
uint32_t constexpr kVisitPeriod = 40;

// If user left the route within this range(meters), adjust the route. Else full rebuild.
double constexpr kAdjustRangeM = 5000.0;
// Full rebuild if distance(meters) is less.
double constexpr kMinDistanceToFinishM = 10000;
// Limit of adjust in seconds.
double constexpr kAdjustLimitSec = 5 * 60;

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

struct ProgressRange final
{
  float const startValue;
  float const stopValue;
};

ProgressRange CalcProgressRange(Checkpoints const & checkpoints, size_t subrouteIdx)
{
  double fullDistance = 0.0;
  double startDistance = 0.0;
  double finishDistance = 0.0;

  for (size_t i = 0; i < checkpoints.GetNumSubroutes(); ++i)
  {
    double const distance =
        MercatorBounds::DistanceOnEarth(checkpoints.GetPoint(i), checkpoints.GetPoint(i + 1));
    fullDistance += distance;
    if (i < subrouteIdx)
      startDistance += distance;
    if (i <= subrouteIdx)
      finishDistance += distance;
  }

  CHECK_GREATER(fullDistance, 0.0, ());
  return {static_cast<float>(startDistance / fullDistance * 100.0),
          static_cast<float>(finishDistance / fullDistance * 100.0)};
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

IRouter::ResultCode IndexRouter::CalculateRoute(Checkpoints const & checkpoints,
                                                m2::PointD const & startDirection,
                                                bool adjustToPrevRoute,
                                                RouterDelegate const & delegate, Route & route)
{
  if (!AllMwmsHaveRoutingIndex(m_index, route))
    return IRouter::ResultCode::FileTooOld;

  auto const & startPoint = checkpoints.GetStart();
  auto const & finalPoint = checkpoints.GetFinish();

  try
  {
    if (adjustToPrevRoute && m_lastRoute && finalPoint == m_lastRoute->GetFinish())
    {
      double const distanceToRoute = m_lastRoute->CalcDistance(startPoint);
      double const distanceToFinish = MercatorBounds::DistanceOnEarth(startPoint, finalPoint);
      if (distanceToRoute <= kAdjustRangeM && distanceToFinish >= kMinDistanceToFinishM)
      {
        auto const code = AdjustRoute(startPoint, startDirection, finalPoint, delegate, route);
        if (code != IRouter::RouteNotFound)
          return code;

        LOG(LWARNING, ("Can't adjust route, do full rebuild, prev start:",
          MercatorBounds::ToLatLon(m_lastRoute->GetStart()), ", start:",
          MercatorBounds::ToLatLon(startPoint), ", finish:",
          MercatorBounds::ToLatLon(finalPoint)));
      }
    }

    return DoCalculateRoute(checkpoints, startDirection, delegate, route);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't find path from", MercatorBounds::ToLatLon(startPoint), "to",
      MercatorBounds::ToLatLon(finalPoint), ":\n ", e.what()));
    return IRouter::InternalError;
  }
}

IRouter::ResultCode IndexRouter::DoCalculateRoute(Checkpoints const & checkpoints,
                                                  m2::PointD const & /* startDirection */,
                                                  RouterDelegate const & delegate, Route & route)
{
  m_lastRoute.reset();

  for (auto const & checkpoint : checkpoints.GetPoints())
  {
    auto const country = platform::CountryFile(m_countryFileFn(checkpoint));
    if (!m_index.IsLoaded(country))
      route.AddAbsentCountry(country.GetName());
  }

  if (!route.GetAbsentCountries().empty())
    return IRouter::NeedMoreMaps;

  TrafficStash::Guard guard(*m_trafficStash);
  WorldGraph graph = MakeWorldGraph();

  vector<Segment> segments;
  segments.push_back(IndexGraphStarter::kStartFakeSegment);

  Segment startSegment;
  if (!FindClosestSegment(checkpoints.GetStart(), true /* isOutgoing */, graph, startSegment))
    return IRouter::StartPointNotFound;

  for (size_t i = checkpoints.GetPassedIdx(); i < checkpoints.GetNumSubroutes(); ++i)
  {
    vector<Segment> subroute;
    auto const result = CalculateSubroute(checkpoints, i, startSegment, delegate, graph, subroute);
    if (result != IRouter::NoError)
      return result;

    IndexGraphStarter::CheckValidRoute(subroute);
    auto const nonFakefinish = IndexGraphStarter::GetNonFakeFinish(subroute);
    startSegment = *nonFakefinish;
    segments.insert(segments.end(), IndexGraphStarter::GetNonFakeStart(subroute), nonFakefinish);
  }

  segments.push_back(startSegment);
  segments.push_back(IndexGraphStarter::kFinishFakeSegment);
  IndexGraphStarter::CheckValidRoute(segments);

  IndexGraphStarter starter(
      IndexGraphStarter::FakeVertex(*IndexGraphStarter::GetNonFakeStart(segments),
                                    checkpoints.GetStart(), false /* strictForward */),
      IndexGraphStarter::FakeVertex(*IndexGraphStarter::GetNonFakeFinish(segments),
                                    checkpoints.GetFinish(), false /* strictForward */),
      graph);

  auto redressResult = RedressRoute(segments, delegate, starter, route);
  if (redressResult != IRouter::NoError)
    return redressResult;

  m_lastRoute = make_unique<SegmentedRoute>(checkpoints.GetStart(), checkpoints.GetFinish());
  for (Segment const & segment : segments)
  {
    if (!IndexGraphStarter::IsFakeSegment(segment))
      m_lastRoute->AddStep(segment, graph.GetPoint(segment, true /* front */));
  }

  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::CalculateSubroute(Checkpoints const & checkpoints,
                                                   size_t subrouteIdx, Segment const & startSegment,
                                                   RouterDelegate const & delegate,
                                                   WorldGraph & graph, vector<Segment> & subroute)
{
  subroute.clear();

  auto const & startPoint = checkpoints.GetPoint(subrouteIdx);
  auto const & finishPoint = checkpoints.GetPoint(subrouteIdx + 1);

  Segment finishSegment;
  if (!FindClosestSegment(finishPoint, false /* isOutgoing */, graph, finishSegment))
  {
    bool const isLastSubroute = subrouteIdx == checkpoints.GetNumSubroutes() - 1;
    return isLastSubroute ? IRouter::EndPointNotFound : IRouter::IntermediatePointNotFound;
  }

  graph.SetMode(AreMwmsNear(startSegment.GetMwmId(), finishSegment.GetMwmId())
                    ? WorldGraph::Mode::LeapsIfPossible
                    : WorldGraph::Mode::LeapsOnly);
  LOG(LINFO, ("Routing in mode:", graph.GetMode()));

  IndexGraphStarter starter(
      IndexGraphStarter::FakeVertex(startSegment, startPoint,
                                    subrouteIdx != checkpoints.GetPassedIdx()),
      IndexGraphStarter::FakeVertex(finishSegment, finishPoint, false /* strictForward */), graph);

  auto const progressRange = CalcProgressRange(checkpoints, subrouteIdx);
  AStarProgress progress(progressRange.startValue, progressRange.stopValue);
  progress.Initialize(starter.GetStartVertex().GetPoint(), starter.GetFinishVertex().GetPoint());

  uint32_t visitCount = 0;

  auto onVisitJunction = [&](Segment const & from, Segment const & to) {
    if (++visitCount % kVisitPeriod != 0)
      return;

    m2::PointD const & pointFrom = starter.GetPoint(from, true /* front */);
    m2::PointD const & pointTo = starter.GetPoint(to, true /* front */);
    auto const lastValue = progress.GetLastValue();
    auto const newValue = progress.GetProgressForBidirectedAlgo(pointFrom, pointTo);
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);

    delegate.OnPointCheck(pointFrom);
  };

  RoutingResult<Segment> routingResult;
  IRouter::ResultCode const result = FindPath(starter.GetStart(), starter.GetFinish(), delegate,
                                              starter, onVisitJunction, routingResult);
  if (result != IRouter::NoError)
    return result;

  IRouter::ResultCode const leapsResult =
      ProcessLeaps(routingResult.path, delegate, graph.GetMode(), starter, subroute);
  if (leapsResult != IRouter::NoError)
    return leapsResult;

  CHECK_GREATER_OR_EQUAL(subroute.size(), routingResult.path.size(), ());
  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::AdjustRoute(m2::PointD const & startPoint,
                                             m2::PointD const & startDirection,
                                             m2::PointD const & finalPoint,
                                             RouterDelegate const & delegate, Route & route)
{
  my::Timer timer;
  TrafficStash::Guard guard(*m_trafficStash);
  WorldGraph graph = MakeWorldGraph();
  graph.SetMode(WorldGraph::Mode::NoLeaps);

  Segment startSegment;
  if (!FindClosestSegment(startPoint, true /* isOutgoing */, graph, startSegment))
    return IRouter::StartPointNotFound;

  auto const & steps = m_lastRoute->GetSteps();
  CHECK(!steps.empty(), ());

  IndexGraphStarter starter(
      IndexGraphStarter::FakeVertex(startSegment, startPoint, false /* strictForward */),
      IndexGraphStarter::FakeVertex(steps.back().GetSegment(), finalPoint,
                                    false /* strictForward */),
      graph);

  AStarProgress progress(0, 100);
  progress.Initialize(starter.GetStartVertex().GetPoint(), starter.GetFinishVertex().GetPoint());

  vector<SegmentEdge> prevEdges;
  for (auto const & step : steps)
    prevEdges.emplace_back(step.GetSegment(), starter.CalcSegmentWeight(step.GetSegment()));

  uint32_t visitCount = 0;

  auto onVisitJunction = [&](Segment const & /* start */, Segment const & vertex) {
    if (visitCount++ % kVisitPeriod != 0)
      return;

    m2::PointD const & point = starter.GetPoint(vertex, true /* front */);
    auto const lastValue = progress.GetLastValue();
    auto const newValue = progress.GetProgressForDirectedAlgo(point);
    if (newValue - lastValue > kProgressInterval)
      delegate.OnProgress(newValue);

    delegate.OnPointCheck(point);
  };

  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Segment> result;
  auto resultCode = ConvertResult<IndexGraphStarter>(algorithm.AdjustRoute(
      starter, starter.GetStart(), prevEdges, kAdjustLimitSec, result, delegate, onVisitJunction));
  if (resultCode != IRouter::NoError)
    return resultCode;

  result.path.push_back(starter.GetFinish());
  auto const redressResult = RedressRoute(result.path, delegate, starter, route);
  if (redressResult != IRouter::NoError)
    return redressResult;

  LOG(LINFO,
      ("Adjust route, elapsed:", timer.ElapsedSeconds(), ", prev start:",
       MercatorBounds::ToLatLon(m_lastRoute->GetStart()), ", start:",
       MercatorBounds::ToLatLon(startPoint), ", finish:", MercatorBounds::ToLatLon(finalPoint),
       ", prev route:", steps.size(), ", new route:", result.path.size()));

  return IRouter::NoError;
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

bool IndexRouter::FindClosestSegment(m2::PointD const & point, bool isOutgoing,
                                     WorldGraph & worldGraph, Segment & closestSegment) const
{
  auto const file = platform::CountryFile(m_countryFileFn(point));
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
                                              RouterDelegate const & delegate,
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
  starter.GetGraph().SetMode(WorldGraph::Mode::NoLeaps);

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
  
  CHECK(m_directionsEngine, ());
  ReconstructRoute(*m_directionsEngine, roadGraph, m_trafficStash, delegate, false /* hasAltitude */, junctions,
                   std::move(times), route);

  if (!route.IsValid())
  {
    LOG(LERROR, ("ReconstructRoute failed. Segments:", segments.size()));
    return IRouter::InternalError;
  }

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
