#include "routing/index_router.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/base/astar_progress.hpp"
#include "routing/base/routing_result.hpp"
#include "routing/bicycle_directions.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/fake_ending.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/pedestrian_directions.hpp"
#include "routing/restriction_loader.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/single_vehicle_world_graph.hpp"
#include "routing/transit_info.hpp"
#include "routing/transit_world_graph.hpp"
#include "routing/turns_generator.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"
#include "routing_common/transit_speed_limits.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/distance.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "platform/mwm_traits.hpp"

#include "base/exception.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <map>
#include <utility>

using namespace routing;
using namespace std;

namespace
{
size_t constexpr kMaxRoadCandidates = 12;
float constexpr kProgressInterval = 2;
uint32_t constexpr kVisitPeriod = 40;

// If user left the route within this range(meters), adjust the route. Else full rebuild.
double constexpr kAdjustRangeM = 5000.0;
// Full rebuild if distance(meters) is less.
double constexpr kMinDistanceToFinishM = 10000;
// Limit of adjust in seconds.
double constexpr kAdjustLimitSec = 5 * 60;

double CalcMaxSpeed(NumMwmIds const & numMwmIds,
                    VehicleModelFactoryInterface const & vehicleModelFactory,
                    VehicleType vehicleType)
{
  if (vehicleType == VehicleType::Transit)
    return transit::kTransitMaxSpeedKMpH;

  double maxSpeed = 0.0;
  numMwmIds.ForEachId([&](NumMwmId id) {
    string const & country = numMwmIds.GetFile(id).GetName();
    double const mwmMaxSpeed =
        vehicleModelFactory.GetVehicleModelForCountry(country)->GetMaxSpeed();
    maxSpeed = max(maxSpeed, mwmMaxSpeed);
  });
  CHECK_GREATER(maxSpeed, 0.0, ());
  return maxSpeed;
}

double CalcOffroadSpeed(VehicleModelFactoryInterface const & vehicleModelFactory)
{
  return vehicleModelFactory.GetVehicleModel()->GetOffroadSpeed();
}

shared_ptr<VehicleModelFactoryInterface> CreateVehicleModelFactory(
    VehicleType vehicleType, CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit:
    return make_shared<PedestrianModelFactory>(countryParentNameGetterFn);
  case VehicleType::Bicycle: return make_shared<BicycleModelFactory>(countryParentNameGetterFn);
  case VehicleType::Car: return make_shared<CarModelFactory>(countryParentNameGetterFn);
  case VehicleType::Count:
    CHECK(false, ("Can't create VehicleModelFactoryInterface for", vehicleType));
    return nullptr;
  }
}

unique_ptr<IDirectionsEngine> CreateDirectionsEngine(VehicleType vehicleType,
                                                     shared_ptr<NumMwmIds> numMwmIds, Index & index)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return make_unique<PedestrianDirectionsEngine>(numMwmIds);
  case VehicleType::Bicycle:
  // @TODO Bicycle turn generation engine is used now. It's ok for the time being.
  // But later a special car turn generation engine should be implemented.
  case VehicleType::Car: return make_unique<BicycleDirectionsEngine>(index, numMwmIds);
  case VehicleType::Count:
    CHECK(false, ("Can't create DirectionsEngine for", vehicleType));
    return nullptr;
  }
}

shared_ptr<TrafficStash> CreateTrafficStash(VehicleType vehicleType, shared_ptr<NumMwmIds> numMwmIds,
                                            traffic::TrafficCache const & trafficCache)
{
  if (vehicleType != VehicleType::Car)
    return nullptr;

  return make_shared<TrafficStash>(trafficCache, numMwmIds);
}

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
    typename Graph::Vertex const & start, typename Graph::Vertex const & finish,
    RouterDelegate const & delegate, Graph & graph,
    typename AStarAlgorithm<Graph>::OnVisitedVertexCallback const & onVisitedVertexCallback,
    typename AStarAlgorithm<Graph>::CheckLengthCallback const & checkLengthCallback,
    RoutingResult<typename Graph::Vertex, typename Graph::Weight> & routingResult)
{
  AStarAlgorithm<Graph> algorithm;
  return ConvertResult<Graph>(algorithm.FindPathBidirectional(
      graph, start, finish, routingResult, delegate, onVisitedVertexCallback, checkLengthCallback));
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
    graph.GetEdgeList(u, isOutgoing, false /* isLeap */, false /* isEnding */, edges);
  };

  return !CheckGraphConnectivity(segment, kDeadEndTestLimit, worldGraph,
                                 getVertexByEdgeFn, getOutgoingEdgesFn);
}

bool MwmHasRoutingData(version::MwmTraits const & traits, VehicleType vehicleType)
{
  if (!traits.HasRoutingIndex())
    return false;

  return vehicleType == VehicleType::Car || traits.HasCrossMwmSection();
}

void GetOutdatedMwms(VehicleType vehicleType, Index & index, vector<string> & outdatedMwms)
{
  outdatedMwms.clear();
  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);

  for (auto const & info : infos)
  {
    if (info->GetType() != MwmInfo::COUNTRY)
      continue;

    if (!MwmHasRoutingData(version::MwmTraits(info->m_version), vehicleType))
      outdatedMwms.push_back(info->GetCountryName());
  }
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

  if (fullDistance == 0.0)
    return {100.0, 100.0};

  return {static_cast<float>(startDistance / fullDistance * 100.0),
          static_cast<float>(finishDistance / fullDistance * 100.0)};
}

void PushPassedSubroutes(Checkpoints const & checkpoints, vector<Route::SubrouteAttrs> & subroutes)
{
  for (size_t i = 0; i < checkpoints.GetPassedIdx(); ++i)
  {
    subroutes.emplace_back(Junction(checkpoints.GetPoint(i), feature::kDefaultAltitudeMeters),
                           Junction(checkpoints.GetPoint(i + 1), feature::kDefaultAltitudeMeters),
                           0 /* beginSegmentIdx */, 0 /* endSegmentIdx */);
  }
}

bool GetLastRealOrPart(IndexGraphStarter const & starter, vector<Segment> const & route,
                       Segment & real)
{
  for (auto rit = route.rbegin(); rit != route.rend(); ++rit)
  {
    real = *rit;
    if (starter.ConvertToReal(real))
      return true;
  }
  return false;
}
}  // namespace

namespace routing
{
// IndexRouter::BestEdgeComparator ----------------------------------------------------------------
IndexRouter::BestEdgeComparator::BestEdgeComparator(m2::PointD const & point, m2::PointD const & direction)
  : m_point(point), m_direction(direction)
{
}

int IndexRouter::BestEdgeComparator::Compare(Edge const & edge1, Edge const & edge2) const
{
  if (IsDirectionValid())
  {
    bool const isEdge1Codirectional = IsAlmostCodirectional(edge1);
    if (isEdge1Codirectional != IsAlmostCodirectional(edge2))
      return isEdge1Codirectional ? -1 : 1;
  }

  double const squaredDistFromEdge1 = GetSquaredDist(edge1);
  double const squaredDistFromEdge2 = GetSquaredDist(edge2);
  if (squaredDistFromEdge1 == squaredDistFromEdge2)
    return 0;

  return squaredDistFromEdge1 < squaredDistFromEdge2 ? -1 : 1;
}

bool IndexRouter::BestEdgeComparator::IsAlmostCodirectional(Edge const & edge) const
{
  CHECK(IsDirectionValid(), ());

  auto const edgeDirection = edge.GetDirection();
  if (edgeDirection.IsAlmostZero())
    return false;

  double const cosAng = m2::DotProduct(m_direction.Normalize(), edgeDirection.Normalize());
  // Note. acos(0.97) ≈ 14 degrees.
  double constexpr kMinCosAngForAlmostCodirectionalVectors = 0.97;
  return cosAng >= kMinCosAngForAlmostCodirectionalVectors;
}

double IndexRouter::BestEdgeComparator::GetSquaredDist(Edge const & edge) const
{
  m2::DistanceToLineSquare<m2::PointD> squaredDistance;
  squaredDistance.SetBounds(edge.GetStartJunction().GetPoint(), edge.GetEndJunction().GetPoint());
  return squaredDistance(m_point);
}

// IndexRouter ------------------------------------------------------------------------------------
IndexRouter::IndexRouter(VehicleType vehicleType, bool loadAltitudes,
                         CountryParentNameGetterFn const & countryParentNameGetterFn,
                         TCountryFileFn const & countryFileFn, CourntryRectFn const & countryRectFn,
                         shared_ptr<NumMwmIds> numMwmIds, unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                         traffic::TrafficCache const & trafficCache, Index & index)
  : m_vehicleType(vehicleType)
  , m_loadAltitudes(loadAltitudes)
  , m_name("astar-bidirectional-" + ToString(m_vehicleType))
  , m_index(index)
  , m_vehicleModelFactory(CreateVehicleModelFactory(m_vehicleType, countryParentNameGetterFn))
  , m_countryFileFn(countryFileFn)
  , m_countryRectFn(countryRectFn)
  , m_numMwmIds(move(numMwmIds))
  , m_numMwmTree(move(numMwmTree))
  , m_trafficStash(CreateTrafficStash(m_vehicleType, m_numMwmIds, trafficCache))
  , m_indexManager(countryFileFn, m_index)
  , m_roadGraph(m_index,
                vehicleType == VehicleType::Pedestrian || vehicleType == VehicleType::Transit
                    ? IRoadGraph::Mode::IgnoreOnewayTag
                    : IRoadGraph::Mode::ObeyOnewayTag,
                m_vehicleModelFactory)
  , m_estimator(EdgeEstimator::Create(
        m_vehicleType, CalcMaxSpeed(*m_numMwmIds, *m_vehicleModelFactory, m_vehicleType),
        CalcOffroadSpeed(*m_vehicleModelFactory), m_trafficStash))
  , m_directionsEngine(CreateDirectionsEngine(m_vehicleType, m_numMwmIds, m_index))
{
  CHECK(!m_name.empty(), ());
  CHECK(m_numMwmIds, ());
  CHECK(m_numMwmTree, ());
  CHECK(m_vehicleModelFactory, ());
  CHECK(m_estimator, ());
  CHECK(m_directionsEngine, ());
}

unique_ptr<WorldGraph> IndexRouter::MakeSingleMwmWorldGraph()
{
  auto worldGraph = MakeWorldGraph();
  worldGraph->SetMode(WorldGraph::Mode::SingleMwm);
  return worldGraph;
}

bool IndexRouter::FindBestSegment(m2::PointD const & point, m2::PointD const & direction,
                                  bool isOutgoing, WorldGraph & worldGraph,
                                  Segment & bestSegment)
{
  bool dummy;
  return FindBestSegment(point, direction, isOutgoing, worldGraph, bestSegment,
                         dummy /* best segment is almost codirectional */);
}

IRouter::ResultCode IndexRouter::CalculateRoute(Checkpoints const & checkpoints,
                                                m2::PointD const & startDirection,
                                                bool adjustToPrevRoute,
                                                RouterDelegate const & delegate, Route & route)
{
  vector<string> outdatedMwms;
  GetOutdatedMwms(m_vehicleType, m_index, outdatedMwms);
  if (!outdatedMwms.empty())
  {
    // Backward compatibility with outdated mwm versions.
    if (m_vehicleType == VehicleType::Pedestrian)
    {
      return CreatePedestrianAStarBidirectionalRouter(m_index, m_countryFileFn, m_numMwmIds)
          ->CalculateRoute(checkpoints, startDirection, adjustToPrevRoute, delegate, route);
    }

    if (m_vehicleType == VehicleType::Bicycle)
    {
      return CreateBicycleAStarBidirectionalRouter(m_index, m_countryFileFn, m_numMwmIds)
          ->CalculateRoute(checkpoints, startDirection, adjustToPrevRoute, delegate, route);
    }

    for (string const & mwm : outdatedMwms)
      route.AddAbsentCountry(mwm);

    return IRouter::ResultCode::FileTooOld;
  }

  auto const & startPoint = checkpoints.GetStart();
  auto const & finalPoint = checkpoints.GetFinish();

  try
  {
    if (adjustToPrevRoute && m_lastRoute && m_lastFakeEdges &&
        finalPoint == m_lastRoute->GetFinish())
    {
      double const distanceToRoute = m_lastRoute->CalcDistance(startPoint);
      double const distanceToFinish = MercatorBounds::DistanceOnEarth(startPoint, finalPoint);
      if (distanceToRoute <= kAdjustRangeM && distanceToFinish >= kMinDistanceToFinishM)
      {
        auto const code = AdjustRoute(checkpoints, startDirection, delegate, route);
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
                                                  m2::PointD const & startDirection,
                                                  RouterDelegate const & delegate, Route & route)
{
  m_lastRoute.reset();

  for (auto const & checkpoint : checkpoints.GetPoints())
  {
    string const countryName = m_countryFileFn(checkpoint);
    if (countryName.empty())
    {
      LOG(LWARNING, ("For point", MercatorBounds::ToLatLon(checkpoint),
                   "CountryInfoGetter returns an empty CountryFile(). It happens when checkpoint"
                   "is put at gaps between mwm."));
      return IRouter::InternalError;
    }

    auto const country = platform::CountryFile(countryName);
    if (!m_index.IsLoaded(country))
      route.AddAbsentCountry(country.GetName());
  }

  if (!route.GetAbsentCountries().empty())
    return IRouter::NeedMoreMaps;

  TrafficStash::Guard guard(m_trafficStash);
  auto graph = MakeWorldGraph();

  vector<Segment> segments;

  Segment startSegment;
  bool startSegmentIsAlmostCodirectionalDirection = false;
  if (!FindBestSegment(checkpoints.GetPointFrom(), startDirection, true /* isOutgoing */, *graph,
                       startSegment, startSegmentIsAlmostCodirectionalDirection))
  {
    return IRouter::StartPointNotFound;
  }

  size_t subrouteSegmentsBegin = 0;
  vector<Route::SubrouteAttrs> subroutes;
  PushPassedSubroutes(checkpoints, subroutes);
  unique_ptr<IndexGraphStarter> starter;

  for (size_t i = checkpoints.GetPassedIdx(); i < checkpoints.GetNumSubroutes(); ++i)
  {
    bool const isFirstSubroute = i == checkpoints.GetPassedIdx();
    bool const isLastSubroute = i == checkpoints.GetNumSubroutes() - 1;
    auto const & startCheckpoint = checkpoints.GetPoint(i);
    auto const & finishCheckpoint = checkpoints.GetPoint(i + 1);

    Segment finishSegment;
    bool dummy = false;
    if (!FindBestSegment(finishCheckpoint, m2::PointD::Zero() /* direction */,
                         false /* isOutgoing */, *graph, finishSegment,
                         dummy /* bestSegmentIsAlmostCodirectional */))
    {
      return isLastSubroute ? IRouter::EndPointNotFound : IRouter::IntermediatePointNotFound;
    }

    bool isStartSegmentStrictForward = m_vehicleType == VehicleType::Car ? true : false;
    if (isFirstSubroute)
      isStartSegmentStrictForward = startSegmentIsAlmostCodirectionalDirection;

    IndexGraphStarter subrouteStarter(MakeFakeEnding(startSegment, startCheckpoint, *graph),
                                      MakeFakeEnding(finishSegment, finishCheckpoint, *graph),
                                      starter ? starter->GetNumFakeSegments() : 0,
                                      isStartSegmentStrictForward, *graph);

    vector<Segment> subroute;
    auto const result =
        CalculateSubroute(checkpoints, i, startSegment, delegate, subrouteStarter, subroute);

    if (result != IRouter::NoError)
      return result;

    IndexGraphStarter::CheckValidRoute(subroute);

    segments.insert(segments.end(), subroute.begin(), subroute.end());

    size_t subrouteSegmentsEnd = segments.size();
    subroutes.emplace_back(subrouteStarter.GetStartJunction(), subrouteStarter.GetFinishJunction(),
                           subrouteSegmentsBegin, subrouteSegmentsEnd);
    subrouteSegmentsBegin = subrouteSegmentsEnd;
    bool const hasRealOrPart = GetLastRealOrPart(subrouteStarter, subroute, startSegment);
    CHECK(hasRealOrPart, ("No real or part of real segments in route."));
    if (!starter)
      starter = make_unique<IndexGraphStarter>(move(subrouteStarter));
    else
      starter->Append(FakeEdgesContainer(move(subrouteStarter)));
  }

  route.SetCurrentSubrouteIdx(checkpoints.GetPassedIdx());
  route.SetSubroteAttrs(move(subroutes));

  IndexGraphStarter::CheckValidRoute(segments);

  auto redressResult = RedressRoute(segments, delegate, *starter, route);
  if (redressResult != IRouter::NoError)
    return redressResult;

  m_lastRoute = make_unique<SegmentedRoute>(checkpoints.GetStart(), checkpoints.GetFinish(),
                                            route.GetSubroutes());
  for (Segment const & segment : segments)
    m_lastRoute->AddStep(segment, starter->GetPoint(segment, true /* front */));

  m_lastFakeEdges = make_unique<FakeEdgesContainer>(move(*starter));

  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::CalculateSubroute(Checkpoints const & checkpoints,
                                                   size_t subrouteIdx, Segment const & startSegment,
                                                   RouterDelegate const & delegate,
                                                   IndexGraphStarter & starter,
                                                   vector<Segment> & subroute)
{
  subroute.clear();

  // We use leaps for cars only. Other vehicle types do not have weights in their cross-mwm sections.
  switch (m_vehicleType)
  {
    case VehicleType::Pedestrian:
    case VehicleType::Bicycle:
    case VehicleType::Transit:
      starter.GetGraph().SetMode(WorldGraph::Mode::NoLeaps);
      break;
    case VehicleType::Car:
      starter.GetGraph().SetMode(AreMwmsNear(starter.GetMwms()) ? WorldGraph::Mode::LeapsIfPossible
                                                                : WorldGraph::Mode::LeapsOnly);
      break;
    case VehicleType::Count:
      CHECK(false, ("Unknown vehicle type:", m_vehicleType));
      break;
  }

  LOG(LINFO, ("Routing in mode:", starter.GetGraph().GetMode()));

  auto const progressRange = CalcProgressRange(checkpoints, subrouteIdx);
  AStarProgress progress(progressRange.startValue, progressRange.stopValue);
  progress.Initialize(starter.GetStartJunction().GetPoint(),
                      starter.GetFinishJunction().GetPoint());

  uint32_t visitCount = 0;
  auto lastValue = progress.GetLastValue();

  auto onVisitJunction = [&](Segment const & from, Segment const & to) {
    if (++visitCount % kVisitPeriod != 0)
      return;

    m2::PointD const & pointFrom = starter.GetPoint(from, true /* front */);
    m2::PointD const & pointTo = starter.GetPoint(to, true /* front */);
    auto const newValue = progress.GetProgressForBidirectedAlgo(pointFrom, pointTo);
    if (newValue - lastValue > kProgressInterval)
    {
      lastValue = newValue;
      delegate.OnProgress(newValue);
    }

    delegate.OnPointCheck(pointFrom);
  };

  RoutingResult<Segment, RouteWeight> routingResult;
  IRouter::ResultCode const result = FindPath(
      starter.GetStartSegment(), starter.GetFinishSegment(), delegate, starter, onVisitJunction,
      [&starter](RouteWeight const & weight) { return starter.CheckLength(weight); },
      routingResult);
  if (result != IRouter::NoError)
    return result;

  IRouter::ResultCode const leapsResult =
      ProcessLeaps(routingResult.m_path, delegate, starter.GetGraph().GetMode(), starter, subroute);
  if (leapsResult != IRouter::NoError)
    return leapsResult;

  CHECK_GREATER_OR_EQUAL(subroute.size(), routingResult.m_path.size(), ());
  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::AdjustRoute(Checkpoints const & checkpoints,
                                             m2::PointD const & startDirection,
                                             RouterDelegate const & delegate, Route & route)
{
  my::Timer timer;
  TrafficStash::Guard guard(m_trafficStash);
  auto graph = MakeWorldGraph();
  graph->SetMode(WorldGraph::Mode::NoLeaps);

  Segment startSegment;
  m2::PointD const & pointFrom = checkpoints.GetPointFrom();
  bool bestSegmentIsAlmostCodirectional = false;
  if (!FindBestSegment(pointFrom, startDirection, true /* isOutgoing */, *graph, startSegment,
                       bestSegmentIsAlmostCodirectional))
    return IRouter::StartPointNotFound;

  auto const & lastSubroutes = m_lastRoute->GetSubroutes();
  CHECK(!lastSubroutes.empty(), ());
  auto const & lastSubroute = m_lastRoute->GetSubroute(checkpoints.GetPassedIdx());

  auto const & steps = m_lastRoute->GetSteps();
  CHECK(!steps.empty(), ());

  FakeEnding dummy;
  IndexGraphStarter starter(MakeFakeEnding(startSegment, pointFrom, *graph), dummy,
                            m_lastFakeEdges->GetNumFakeEdges(), bestSegmentIsAlmostCodirectional,
                            *graph);

  starter.Append(*m_lastFakeEdges);

  AStarProgress progress(0, 95);
  progress.Initialize(starter.GetStartJunction().GetPoint(),
                      starter.GetFinishJunction().GetPoint());

  vector<SegmentEdge> prevEdges;
  CHECK_LESS_OR_EQUAL(lastSubroute.GetEndSegmentIdx(), steps.size(), ());
  for (size_t i = lastSubroute.GetBeginSegmentIdx(); i < lastSubroute.GetEndSegmentIdx(); ++i)
  {
    auto const & step = steps[i];
    prevEdges.emplace_back(step.GetSegment(), starter.CalcSegmentWeight(step.GetSegment()));
  }

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

  auto const checkLength = [&starter](RouteWeight const & weight) {
    return weight <= RouteWeight(kAdjustLimitSec) && starter.CheckLength(weight);
  };

  AStarAlgorithm<IndexGraphStarter> algorithm;
  RoutingResult<Segment, RouteWeight> result;
  auto resultCode = ConvertResult<IndexGraphStarter>(
      algorithm.AdjustRoute(starter, starter.GetStartSegment(), prevEdges, result, delegate,
                            onVisitJunction, checkLength));
  if (resultCode != IRouter::NoError)
    return resultCode;

  CHECK_GREATER_OR_EQUAL(result.m_path.size(), 2, ());
  CHECK(IndexGraphStarter::IsFakeSegment(result.m_path.front()), ());
  CHECK(IndexGraphStarter::IsFakeSegment(result.m_path.back()), ());

  vector<Route::SubrouteAttrs> subroutes;
  PushPassedSubroutes(checkpoints, subroutes);

  size_t subrouteOffset = result.m_path.size();
  subroutes.emplace_back(starter.GetStartJunction(), starter.GetFinishJunction(),
                         0 /* beginSegmentIdx */, subrouteOffset);

  for (size_t i = checkpoints.GetPassedIdx() + 1; i < lastSubroutes.size(); ++i)
  {
    auto const & subroute = lastSubroutes[i];

    for (size_t j = subroute.GetBeginSegmentIdx(); j < subroute.GetEndSegmentIdx(); ++j)
      result.m_path.push_back(steps[j].GetSegment());

    subroutes.emplace_back(subroute, subrouteOffset);
    subrouteOffset = subroutes.back().GetEndSegmentIdx();
  }

  CHECK_EQUAL(result.m_path.size(), subrouteOffset, ());

  route.SetCurrentSubrouteIdx(checkpoints.GetPassedIdx());
  route.SetSubroteAttrs(move(subroutes));

  auto const redressResult = RedressRoute(result.m_path, delegate, starter, route);
  if (redressResult != IRouter::NoError)
    return redressResult;

  LOG(LINFO, ("Adjust route, elapsed:", timer.ElapsedSeconds(), ", prev start:", checkpoints,
              ", prev route:", steps.size(), ", new route:", result.m_path.size()));

  return IRouter::NoError;
}

unique_ptr<WorldGraph> IndexRouter::MakeWorldGraph()
{
  auto crossMwmGraph = make_unique<CrossMwmGraph>(
      m_numMwmIds, m_numMwmTree, m_vehicleModelFactory,
      m_vehicleType == VehicleType::Transit ? VehicleType::Pedestrian : m_vehicleType,
      m_countryRectFn, m_index, m_indexManager);
  auto indexGraphLoader = IndexGraphLoader::Create(
      m_vehicleType == VehicleType::Transit ? VehicleType::Pedestrian : m_vehicleType,
      m_loadAltitudes, m_numMwmIds, m_vehicleModelFactory, m_estimator, m_index);
  if (m_vehicleType != VehicleType::Transit)
    return make_unique<SingleVehicleWorldGraph>(move(crossMwmGraph), move(indexGraphLoader),
                                                m_estimator);
  auto transitGraphLoader = TransitGraphLoader::Create(m_index, m_numMwmIds, m_estimator);
  return make_unique<TransitWorldGraph>(move(crossMwmGraph), move(indexGraphLoader),
                                        move(transitGraphLoader), m_estimator);
}

bool IndexRouter::FindBestSegment(m2::PointD const & point, m2::PointD const & direction,
                                  bool isOutgoing, WorldGraph & worldGraph, Segment & bestSegment,
                                  bool & bestSegmentIsAlmostCodirectional) const
{
  auto const file = platform::CountryFile(m_countryFileFn(point));
  MwmSet::MwmHandle handle = m_index.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(MwmIsNotAliveException, ("Can't get mwm handle for", file));

  auto const mwmId = MwmSet::MwmId(handle.GetInfo());
  NumMwmId const numMwmId = m_numMwmIds->GetId(file);

  vector<pair<Edge, Junction>> candidates;
  m_roadGraph.FindClosestEdges(point, kMaxRoadCandidates, candidates);

  auto const getSegmentByEdge = [&numMwmId](Edge const & edge) {
    return Segment(numMwmId, edge.GetFeatureId().m_index, edge.GetSegId(), edge.IsForward());
  };

  // Getting rid of knowingly bad candidates.
  my::EraseIf(candidates, [&](pair<Edge, Junction> const & p) {
    Edge const & edge = p.first;
    return edge.GetFeatureId().m_mwmId != mwmId || IsDeadEnd(getSegmentByEdge(edge), isOutgoing, worldGraph);
  });

  if (candidates.empty())
    return false;

  BestEdgeComparator bestEdgeComparator(point, direction);
  Edge bestEdge = candidates[0].first;
  for (size_t i = 1; i < candidates.size(); ++i)
  {
    Edge const & edge = candidates[i].first;
    if (bestEdgeComparator.Compare(edge, bestEdge) < 0)
      bestEdge = edge;
  }

  bestSegmentIsAlmostCodirectional =
      bestEdgeComparator.IsDirectionValid() && bestEdgeComparator.IsAlmostCodirectional(bestEdge);
  bestSegment = getSegmentByEdge(bestEdge);
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

  for (size_t i = 0; i < input.size(); ++i)
  {
    Segment current = input[i];

    if ((prevMode == WorldGraph::Mode::LeapsOnly && !starter.ConvertToReal(current)) ||
        (prevMode != WorldGraph::Mode::LeapsOnly && !starter.IsLeap(current.GetMwmId())))
    {
      output.push_back(current);
      continue;
    }

    // Clear previous loaded graphs to not spend too much memory at one time.
    worldGraph.ClearCachedGraphs();

    ++i;
    CHECK_LESS(i, input.size(), ());
    Segment next = input[i];
    starter.ConvertToReal(next);

    CHECK(!IndexGraphStarter::IsFakeSegment(current), ());
    CHECK(!IndexGraphStarter::IsFakeSegment(next), ());
    CHECK_EQUAL(
      current.GetMwmId(), next.GetMwmId(),
      ("Different mwm ids for leap enter and exit, i:", i, "size of input:", input.size()));

    IRouter::ResultCode result = IRouter::InternalError;
    RoutingResult<Segment, RouteWeight> routingResult;
    // In case of leaps from the start to its mwm transition and from finish mwm transition
    // route calculation should be made on the world graph (WorldGraph::Mode::NoLeaps).
    if (starter.GetMwms().count(current.GetMwmId()) && prevMode == WorldGraph::Mode::LeapsOnly)
    {
      // World graph route.
      worldGraph.SetMode(WorldGraph::Mode::NoLeaps);
    }
    else
    {
      // Single mwm route.
      worldGraph.SetMode(WorldGraph::Mode::SingleMwm);
    }

    result =
        FindPath(current, next, delegate, worldGraph, {} /* onVisitedVertexCallback */,
                 [&starter](RouteWeight const & weight) { return starter.CheckLength(weight); },
                 routingResult);

    if (result != IRouter::NoError)
      return result;

    // Start and finish segments may be changed by starter.ConvertToReal. It was necessary to use it
    // in worldGraph but we need to reset them to original values.
    routingResult.m_path.front() = input[i - 1];
    routingResult.m_path.back() = input[i];

    output.insert(output.end(), routingResult.m_path.cbegin(), routingResult.m_path.cend());
  }

  return IRouter::NoError;
}

IRouter::ResultCode IndexRouter::RedressRoute(vector<Segment> const & segments,
                                              RouterDelegate const & delegate,
                                              IndexGraphStarter & starter, Route & route) const
{
  CHECK(!segments.empty(), ());
  vector<Junction> junctions;
  size_t const numPoints = IndexGraphStarter::GetRouteNumPoints(segments);
  junctions.reserve(numPoints);

  for (size_t i = 0; i < numPoints; ++i)
    junctions.emplace_back(starter.GetRouteJunction(segments, i));

  IndexRoadGraph roadGraph(m_numMwmIds, starter, segments, junctions, m_index);
  starter.GetGraph().SetMode(WorldGraph::Mode::NoLeaps);

  Route::TTimes times;
  times.reserve(segments.size());
  double time = 0.0;
  times.emplace_back(static_cast<uint32_t>(0), 0.0);

  for (size_t i = 0; i + 1 < numPoints; ++i)
  {
    time += starter.CalcRouteSegmentWeight(segments, i).GetWeight();
    times.emplace_back(static_cast<uint32_t>(i + 1), time);
  }

  CHECK(m_directionsEngine, ());
  ReconstructRoute(*m_directionsEngine, roadGraph, m_trafficStash, delegate, junctions, move(times),
                   route);

  auto & worldGraph = starter.GetGraph();
  for (auto & routeSegment : route.GetRouteSegments())
    routeSegment.SetTransitInfo(worldGraph.GetTransitInfo(routeSegment.GetSegment()));

  if (!route.IsValid())
  {
    LOG(LERROR, ("ReconstructRoute failed. Segments:", segments.size()));
    return IRouter::InternalError;
  }

  if (delegate.IsCancelled())
    return IRouter::Cancelled;

  return IRouter::NoError;
}

bool IndexRouter::AreMwmsNear(set<NumMwmId> const & mwmIds) const
{
  for (auto const & outerId : mwmIds)
  {
    m2::RectD const rect = m_countryRectFn(m_numMwmIds->GetFile(outerId).GetName());
    size_t found = 0;
    m_numMwmTree->ForEachInRect(rect, [&](NumMwmId id) { found += mwmIds.count(id); });
    if (found != mwmIds.size())
      return false;
  }
  return true;
}
}  // namespace routing
