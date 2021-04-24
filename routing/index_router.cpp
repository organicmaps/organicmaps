#include "routing/index_router.hpp"

#include "routing/base/astar_progress.hpp"
#include "routing/base/bfs.hpp"
#include "routing/car_directions.hpp"
#include "routing/fake_ending.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_starter.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/junction_visitor.hpp"
#include "routing/leaps_graph.hpp"
#include "routing/leaps_postprocessor.hpp"
#include "routing/mwm_hierarchy_handler.hpp"
#include "routing/pedestrian_directions.hpp"
#include "routing/route.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/routing_options.hpp"
#include "routing/single_vehicle_world_graph.hpp"
#include "routing/speed_camera_prohibition.hpp"
#include "routing/transit_world_graph.hpp"
#include "routing/vehicle_mask.hpp"

#include "transit/transit_entities.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/data_source.hpp"
#include "indexer/scales.hpp"

#include "platform/mwm_traits.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <limits>
#include <map>
#include <optional>

using namespace routing;
using namespace std;

namespace
{
size_t constexpr kMaxRoadCandidates = 10;
uint32_t constexpr kVisitPeriodForLeaps = 10;
uint32_t constexpr kVisitPeriod = 40;

double constexpr kLeapsStageContribution = 0.15;
double constexpr kAlmostZeroContribution = 1e-7;

// If user left the route within this range(meters), adjust the route. Else full rebuild.
double constexpr kAdjustRangeM = 5000.0;
// Full rebuild if distance(meters) is less.
double constexpr kMinDistanceToFinishM = 10000;

double CalcMaxSpeed(NumMwmIds const & numMwmIds,
                    VehicleModelFactoryInterface const & vehicleModelFactory,
                    VehicleType vehicleType)
{
  if (vehicleType == VehicleType::Transit)
    return kTransitMaxSpeedKMpH;

  double maxSpeed = 0.0;
  numMwmIds.ForEachId([&](NumMwmId id) {
    string const & country = numMwmIds.GetFile(id).GetName();
    double const mwmMaxSpeed =
        vehicleModelFactory.GetVehicleModelForCountry(country)->GetMaxWeightSpeed();
    maxSpeed = max(maxSpeed, mwmMaxSpeed);
  });
  CHECK_GREATER(maxSpeed, 0.0, ("Most likely |numMwmIds| is empty."));
  return maxSpeed;
}

SpeedKMpH const & CalcOffroadSpeed(VehicleModelFactoryInterface const & vehicleModelFactory)
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
  UNREACHABLE();
}

unique_ptr<DirectionsEngine> CreateDirectionsEngine(VehicleType vehicleType,
                                                    shared_ptr<NumMwmIds> numMwmIds,
                                                    DataSource & dataSource)
{
  switch (vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Transit: return make_unique<PedestrianDirectionsEngine>(dataSource, numMwmIds);
  case VehicleType::Bicycle:
  case VehicleType::Car: return make_unique<CarDirectionsEngine>(dataSource, numMwmIds);
  case VehicleType::Count:
    CHECK(false, ("Can't create DirectionsEngine for", vehicleType));
    return nullptr;
  }
  UNREACHABLE();
}

shared_ptr<TrafficStash> CreateTrafficStash(VehicleType vehicleType, shared_ptr<NumMwmIds> numMwmIds,
                                            traffic::TrafficCache const & trafficCache)
{
  if (vehicleType != VehicleType::Car)
    return nullptr;

  return make_shared<TrafficStash>(trafficCache, numMwmIds);
}

/// \returns true if the mwm is ready for index graph routing and cross mwm index graph routing.
/// It means the mwm contains routing and cross_mwm sections. In terms of production mwms
/// the method returns false for mwms 170328 and earlier, and returns true for mwms 170428 and
/// later.
bool MwmHasRoutingData(version::MwmTraits const & traits)
{
  return traits.HasRoutingIndex() && traits.HasCrossMwmSection();
}

void GetOutdatedMwms(DataSource & dataSource, vector<string> & outdatedMwms)
{
  outdatedMwms.clear();
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);

  for (auto const & info : infos)
  {
    if (info->GetType() != MwmInfo::COUNTRY)
      continue;

    if (!MwmHasRoutingData(version::MwmTraits(info->m_version)))
      outdatedMwms.push_back(info->GetCountryName());
  }
}

void PushPassedSubroutes(Checkpoints const & checkpoints, vector<Route::SubrouteAttrs> & subroutes)
{
  for (size_t i = 0; i < checkpoints.GetPassedIdx(); ++i)
  {
    subroutes.emplace_back(
        geometry::PointWithAltitude(checkpoints.GetPoint(i), geometry::kDefaultAltitudeMeters),
        geometry::PointWithAltitude(checkpoints.GetPoint(i + 1), geometry::kDefaultAltitudeMeters),
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

// Returns true if seg1 and seg2 have the same geometry.
bool IsTheSameSegments(m2::PointD const & seg1From, m2::PointD const & seg1To,
                       m2::PointD const & seg2From, m2::PointD const & seg2To)
{
  return (seg1From == seg2From && seg1To == seg2To) || (seg1From == seg2To && seg1To == seg2From);
}

bool IsDeadEnd(Segment const & segment, bool isOutgoing, bool useRoutingOptions,
               WorldGraph & worldGraph, set<Segment> & visitedSegments)
{
  // Maximum size (in Segment) of an island in road graph which may be found by the method.
  size_t constexpr kDeadEndTestLimit = 250;

  return !CheckGraphConnectivity(segment, isOutgoing, useRoutingOptions, kDeadEndTestLimit,
                                 worldGraph, visitedSegments);
}

bool IsDeadEndCached(Segment const & segment, bool isOutgoing, bool useRoutingOptions,
                     WorldGraph & worldGraph, set<Segment> & deadEnds)
{
  if (deadEnds.count(segment) != 0)
    return true;

  set<Segment> visitedSegments;
  if (IsDeadEnd(segment, isOutgoing, useRoutingOptions, worldGraph, visitedSegments))
  {
    auto const beginIt = make_move_iterator(visitedSegments.begin());
    auto const endIt = make_move_iterator(visitedSegments.end());
    deadEnds.insert(beginIt, endIt);
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
  m2::ParametrizedSegment<m2::PointD> const segment(edge.GetStartJunction().GetPoint(),
                                                    edge.GetEndJunction().GetPoint());
  return segment.SquaredDistanceToPoint(m_point);
}

// IndexRouter ------------------------------------------------------------------------------------
IndexRouter::IndexRouter(VehicleType vehicleType, bool loadAltitudes,
                         CountryParentNameGetterFn const & countryParentNameGetterFn,
                         TCountryFileFn const & countryFileFn, CourntryRectFn const & countryRectFn,
                         shared_ptr<NumMwmIds> numMwmIds, unique_ptr<m4::Tree<NumMwmId>> numMwmTree,
                         traffic::TrafficCache const & trafficCache, DataSource & dataSource)
  : m_vehicleType(vehicleType)
  , m_loadAltitudes(loadAltitudes)
  , m_name("astar-bidirectional-" + ToString(m_vehicleType))
  , m_dataSource(dataSource)
  , m_vehicleModelFactory(CreateVehicleModelFactory(m_vehicleType, countryParentNameGetterFn))
  , m_countryFileFn(countryFileFn)
  , m_countryRectFn(countryRectFn)
  , m_numMwmIds(move(numMwmIds))
  , m_numMwmTree(move(numMwmTree))
  , m_trafficStash(CreateTrafficStash(m_vehicleType, m_numMwmIds, trafficCache))
  , m_roadGraph(m_dataSource,
                vehicleType == VehicleType::Pedestrian || vehicleType == VehicleType::Transit
                    ? IRoadGraph::Mode::IgnoreOnewayTag
                    : IRoadGraph::Mode::ObeyOnewayTag,
                m_vehicleModelFactory)
  , m_estimator(EdgeEstimator::Create(
        m_vehicleType, CalcMaxSpeed(*m_numMwmIds, *m_vehicleModelFactory, m_vehicleType),
        CalcOffroadSpeed(*m_vehicleModelFactory), m_trafficStash,
        &dataSource, m_numMwmIds))
  , m_directionsEngine(CreateDirectionsEngine(m_vehicleType, m_numMwmIds, m_dataSource))
  , m_countryParentNameGetterFn(countryParentNameGetterFn)
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
  worldGraph->SetMode(WorldGraphMode::SingleMwm);
  return worldGraph;
}

bool IndexRouter::FindBestSegments(m2::PointD const & checkpoint, m2::PointD const & direction,
                                   bool isOutgoing, WorldGraph & worldGraph,
                                   vector<Segment> & bestSegments)
{
  bool dummy;
  return FindBestSegments(checkpoint, direction, isOutgoing, worldGraph, bestSegments,
                          dummy /* best segment is almost codirectional */);
}

void IndexRouter::ClearState()
{
  m_roadGraph.ClearState();
  m_directionsEngine->Clear();
}

bool IndexRouter::FindClosestProjectionToRoad(m2::PointD const & point,
                                              m2::PointD const & direction, double radius,
                                              EdgeProj & proj)
{
  auto const rect = mercator::RectByCenterXYAndSizeInMeters(point, radius);
  std::vector<std::pair<Edge, geometry::PointWithAltitude>> candidates;

  uint32_t const count = direction.IsAlmostZero() ? 1 : 4;
  m_roadGraph.FindClosestEdges(rect, count, candidates);

  if (candidates.empty())
    return false;

  if (direction.IsAlmostZero())
  {
    // We have no direction so return the first closest edge.
    proj.m_edge = candidates[0].first;
    proj.m_point = candidates[0].second.GetPoint();
    return true;
  }

  // We have direction so we can find the closest codirectional edge.
  Edge codirectionalEdge;
  if (!FindClosestCodirectionalEdge(point, direction, candidates, codirectionalEdge))
    return false;

  for (auto const & [edge, projection] : candidates)
  {
    if (edge == codirectionalEdge)
    {
      proj.m_edge = edge;
      proj.m_point = projection.GetPoint();
      break;
    }
  }

  return true;
}

void IndexRouter::SetGuides(GuidesTracks && guides) { m_guides = GuidesConnections(guides); }

RouterResultCode IndexRouter::CalculateRoute(Checkpoints const & checkpoints,
                                             m2::PointD const & startDirection,
                                             bool adjustToPrevRoute,
                                             RouterDelegate const & delegate, Route & route)
{
  vector<string> outdatedMwms;
  GetOutdatedMwms(m_dataSource, outdatedMwms);

  if (!outdatedMwms.empty())
  {
    for (string const & mwm : outdatedMwms)
      route.AddAbsentCountry(mwm);

    return RouterResultCode::FileTooOld;
  }

  auto const & startPoint = checkpoints.GetStart();
  auto const & finalPoint = checkpoints.GetFinish();

  try
  {
    SCOPE_GUARD(featureRoadGraphClear, [this]{
      this->ClearState();
    });

    if (adjustToPrevRoute && m_lastRoute && m_lastFakeEdges &&
        finalPoint == m_lastRoute->GetFinish())
    {
      double const distanceToRoute = m_lastRoute->CalcDistance(startPoint);
      double const distanceToFinish = mercator::DistanceOnEarth(startPoint, finalPoint);
      if (distanceToRoute <= kAdjustRangeM && distanceToFinish >= kMinDistanceToFinishM)
      {
        auto const code = AdjustRoute(checkpoints, startDirection, delegate, route);
        if (code != RouterResultCode::RouteNotFound)
          return code;

        LOG(LWARNING, ("Can't adjust route, do full rebuild, prev start:",
                       mercator::ToLatLon(m_lastRoute->GetStart()), ", start:", mercator::ToLatLon(startPoint), ", finish:", mercator::ToLatLon(finalPoint)));
      }
    }

    return DoCalculateRoute(checkpoints, startDirection, delegate, route);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't find path from", mercator::ToLatLon(startPoint), "to",
                 mercator::ToLatLon(finalPoint), ":\n ", e.what()));
    return RouterResultCode::InternalError;
  }
}

std::vector<Segment> IndexRouter::GetBestSegments(m2::PointD const & checkpoint, WorldGraph & graph)
{
  bool startSegmentIsAlmostCodirectional = false;
  std::vector<Segment> segments;

  FindBestSegments(checkpoint, m2::PointD::Zero() /* startDirection */, true /* isOutgoing */,
                   graph, segments, startSegmentIsAlmostCodirectional);

  return segments;
}

void IndexRouter::AppendPartsOfReal(LatLonWithAltitude const & point1,
                                    LatLonWithAltitude const & point2, uint32_t & startIdx,
                                    ConnectionToOsm & link)
{
  uint32_t const fakeFeatureId = FakeFeatureIds::kIndexGraphStarterId;

  FakeVertex vertexForward(m_guides.GetMwmId(), point1 /* from */, point2 /* to */,
                           FakeVertex::Type::PartOfReal);

  FakeVertex vertexBackward(m_guides.GetMwmId(), point2 /* from */, point1 /* to */,
                            FakeVertex::Type::PartOfReal);

  link.m_partsOfReal.emplace_back(
      vertexForward, Segment(kFakeNumMwmId, fakeFeatureId, startIdx++, true /* forward */));
  link.m_partsOfReal.emplace_back(
      vertexBackward, Segment(kFakeNumMwmId, fakeFeatureId, startIdx++, true /* forward */));
}

void IndexRouter::ConnectCheckpointsOnGuidesToOsm(std::vector<m2::PointD> const & checkpoints,
                                                  WorldGraph & graph)
{
  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    if (!m_guides.IsCheckpointAttached(checkpointIdx))
      continue;

    if (m_guides.FitsForDirectLinkToGuide(checkpointIdx, checkpoints.size()))
      continue;

    // Projection of this checkpoint is not connected to OSM.
    if (m_guides.GetOsmConnections(checkpointIdx).empty())
      continue;

    auto const & checkpoint = checkpoints[checkpointIdx];
    auto const & bestSegmentsOsm = GetBestSegments(checkpoint, graph);
    if (bestSegmentsOsm.empty())
      continue;

    m_guides.OverwriteFakeEnding(checkpointIdx, MakeFakeEnding(bestSegmentsOsm, checkpoint, graph));
  }
}

uint32_t IndexRouter::ConnectTracksOnGuidesToOsm(std::vector<m2::PointD> const & checkpoints,
                                                 WorldGraph & graph)
{
  uint32_t segmentIdx = 0;

  for (size_t checkpointIdx = 0; checkpointIdx < checkpoints.size(); ++checkpointIdx)
  {
    auto osmConnections = m_guides.GetOsmConnections(checkpointIdx);

    if (osmConnections.empty())
      continue;

    bool foundSegmentsForProjection = false;
    for (size_t i = 0; i < osmConnections.size(); ++i)
    {
      auto & link = osmConnections[i];
      geometry::PointWithAltitude const & proj = link.m_projectedPoint;

      auto const & segmentsProj = GetBestSegments(proj.GetPoint(), graph);
      if (segmentsProj.empty())
        continue;

      if (link.m_fromCheckpoint)
        foundSegmentsForProjection = true;

      auto newFakeEndingProj = MakeFakeEnding(segmentsProj, proj.GetPoint(), graph);

      if (link.m_fakeEnding.m_projections.empty())
        link.m_fakeEnding = newFakeEndingProj;
      else
        GuidesConnections::ExtendFakeEndingProjections(newFakeEndingProj, link.m_fakeEnding);

      LatLonWithAltitude const loopPoint = link.m_loopVertex.GetJunctionTo();

      if (!(loopPoint == link.m_realFrom))
        AppendPartsOfReal(link.m_realFrom, loopPoint, segmentIdx, link);

      if (!(loopPoint == link.m_realTo))
        AppendPartsOfReal(loopPoint, link.m_realTo, segmentIdx, link);
    }
    if (foundSegmentsForProjection)
      m_guides.UpdateOsmConnections(checkpointIdx, osmConnections);
    else
      m_guides.UpdateOsmConnections(checkpointIdx, {});
  }
  return segmentIdx;
}

void IndexRouter::AddGuidesOsmConnectionsToGraphStarter(size_t checkpointIdxFrom,
                                                        size_t checkpointIdxTo,
                                                        IndexGraphStarter & starter)
{
  auto linksFrom = m_guides.GetOsmConnections(checkpointIdxFrom);
  auto linksTo = m_guides.GetOsmConnections(checkpointIdxTo);
  for (auto const & link : linksTo)
  {
    auto it = find_if(linksFrom.begin(), linksFrom.end(), [&link](ConnectionToOsm const & cur) {
      return link.m_fakeEnding.m_originJunction == cur.m_fakeEnding.m_originJunction &&
             link.m_fakeEnding.m_projections == cur.m_fakeEnding.m_projections &&
             link.m_realSegment == cur.m_realSegment && link.m_realTo == cur.m_realTo;
    });

    if (it == linksFrom.end())
    {
      linksFrom.push_back(link);
    }
  }

  for (auto & connOsm : linksFrom)
  {
    if (connOsm.m_fakeEnding.m_projections.empty())
      continue;

    starter.AddEnding(connOsm.m_fakeEnding);

    starter.ConnectLoopToGuideSegments(connOsm.m_loopVertex, connOsm.m_realSegment,
                                       connOsm.m_realFrom, connOsm.m_realTo, connOsm.m_partsOfReal);
  }
}

RouterResultCode IndexRouter::DoCalculateRoute(Checkpoints const & checkpoints,
                                               m2::PointD const & startDirection,
                                               RouterDelegate const & delegate, Route & route)
{
  m_lastRoute.reset();
  // MwmId used for guides segments in RedressRoute().
  NumMwmId guidesMwmId = kFakeNumMwmId;

  for (auto const & checkpoint : checkpoints.GetPoints())
  {
    string const countryName = m_countryFileFn(checkpoint);

    if (countryName.empty())
    {
      LOG(LWARNING, ("For point", mercator::ToLatLon(checkpoint),
                   "CountryInfoGetter returns an empty CountryFile(). It happens when checkpoint"
                   "is put at gaps between mwm."));
      return RouterResultCode::InternalError;
    }

    auto const country = platform::CountryFile(countryName);
    if (!m_dataSource.IsLoaded(country))
    {
      route.AddAbsentCountry(country.GetName());
    }
    else if (guidesMwmId == kFakeNumMwmId)
    {
      guidesMwmId = m_numMwmIds->GetId(country);
    };
  }

  if (!route.GetAbsentCountries().empty())
    return RouterResultCode::NeedMoreMaps;

  TrafficStash::Guard guard(m_trafficStash);
  unique_ptr<WorldGraph> graph = MakeWorldGraph();

  vector<Segment> segments;

  vector<Segment> startSegments;
  bool startSegmentIsAlmostCodirectionalDirection = false;
  bool const foundStart =
      FindBestSegments(checkpoints.GetPointFrom(), startDirection, true /* isOutgoing */, *graph,
                       startSegments, startSegmentIsAlmostCodirectionalDirection);

  m_guides.SetGuidesGraphParams(guidesMwmId, m_estimator->GetMaxWeightSpeedMpS());
  m_guides.ConnectToGuidesGraph(checkpoints.GetPoints());

  if (!m_guides.IsActive() && !foundStart)
  {
    return RouterResultCode::StartPointNotFound;
  }

  uint32_t const startIdx = ConnectTracksOnGuidesToOsm(checkpoints.GetPoints(), *graph);
  ConnectCheckpointsOnGuidesToOsm(checkpoints.GetPoints(), *graph);

  size_t subrouteSegmentsBegin = 0;
  vector<Route::SubrouteAttrs> subroutes;
  PushPassedSubroutes(checkpoints, subroutes);

  unique_ptr<IndexGraphStarter> starter;

  auto progress = make_shared<AStarProgress>();
  double const checkpointsLength = checkpoints.GetSummaryLengthBetweenPointsMeters();

  for (size_t i = checkpoints.GetPassedIdx(); i < checkpoints.GetNumSubroutes(); ++i)
  {
    bool const isFirstSubroute = i == checkpoints.GetPassedIdx();
    bool const isLastSubroute = i == checkpoints.GetNumSubroutes() - 1;
    auto const & startCheckpoint = checkpoints.GetPoint(i);
    auto const & finishCheckpoint = checkpoints.GetPoint(i + 1);

    FakeEnding finishFakeEnding = m_guides.GetFakeEnding(i + 1);

    vector<Segment> finishSegments;
    bool dummy = false;

    // Stop building route if |finishCheckpoint| is not connected to OSM and is not connected to
    // the guides graph.
    if (!FindBestSegments(finishCheckpoint, m2::PointD::Zero() /* direction */,
                          false /* isOutgoing */, *graph, finishSegments,
                          dummy /* bestSegmentIsAlmostCodirectional */) &&
        finishFakeEnding.m_projections.empty())
    {
      return isLastSubroute ? RouterResultCode::EndPointNotFound
                            : RouterResultCode::IntermediatePointNotFound;
    }

    bool isStartSegmentStrictForward = (m_vehicleType == VehicleType::Car);
    if (isFirstSubroute)
      isStartSegmentStrictForward = startSegmentIsAlmostCodirectionalDirection;

    FakeEnding startFakeEnding = m_guides.GetFakeEnding(i);

    if (startFakeEnding.m_projections.empty())
      startFakeEnding = MakeFakeEnding(startSegments, startCheckpoint, *graph);

    if (finishFakeEnding.m_projections.empty())
      finishFakeEnding = MakeFakeEnding(finishSegments, finishCheckpoint, *graph);

    uint32_t const fakeNumerationStart =
        starter ? starter->GetNumFakeSegments() + startIdx : startIdx;
    IndexGraphStarter subrouteStarter(startFakeEnding, finishFakeEnding, fakeNumerationStart,
                                      isStartSegmentStrictForward, *graph);

    if (m_guides.IsAttached())
    {
      subrouteStarter.SetGuides(m_guides.GetGuidesGraph());
      AddGuidesOsmConnectionsToGraphStarter(i, i + 1, subrouteStarter);
    }

    vector<Segment> subroute;
    double contributionCoef = kAlmostZeroContribution;
    if (!base::AlmostEqualAbs(checkpointsLength, 0.0, 1e-5))
    {
      contributionCoef =
          mercator::DistanceOnEarth(startCheckpoint, finishCheckpoint) / checkpointsLength;
    }

    AStarSubProgress subProgress(mercator::ToLatLon(startCheckpoint),
                                 mercator::ToLatLon(finishCheckpoint), contributionCoef);
    progress->AppendSubProgress(subProgress);
    SCOPE_GUARD(eraseProgress, [&progress]() { progress->PushAndDropLastSubProgress(); });

    auto const result = CalculateSubroute(checkpoints, i, delegate, progress, subrouteStarter,
                                          subroute, m_guides.IsAttached());

    if (result != RouterResultCode::NoError)
      return result;

    IndexGraphStarter::CheckValidRoute(subroute);

    segments.insert(segments.end(), subroute.begin(), subroute.end());

    size_t subrouteSegmentsEnd = segments.size();
    subroutes.emplace_back(subrouteStarter.GetStartJunction().ToPointWithAltitude(),
                           subrouteStarter.GetFinishJunction().ToPointWithAltitude(),
                           subrouteSegmentsBegin, subrouteSegmentsEnd);
    subrouteSegmentsBegin = subrouteSegmentsEnd;
    // For every subroute except for the first one the last real segment is used  as a start
    // segment. It's implemented this way to prevent jumping from one road to another one using a
    // via point.
    startSegments.resize(1);
    bool const hasRealOrPart = GetLastRealOrPart(subrouteStarter, subroute, startSegments[0]);
    CHECK(hasRealOrPart, ("No real or part of real segments in route."));
    if (!starter)
      starter = make_unique<IndexGraphStarter>(move(subrouteStarter));
    else
      starter->Append(FakeEdgesContainer(move(subrouteStarter)));
  }

  route.SetCurrentSubrouteIdx(checkpoints.GetPassedIdx());
  route.SetSubroteAttrs(move(subroutes));

  IndexGraphStarter::CheckValidRoute(segments);

  // TODO (@gmoryes) https://jira.mail.ru/browse/MAPSME-10694
  //  We should do RedressRoute for each subroute separately.
  auto redressResult = RedressRoute(segments, delegate.GetCancellable(), *starter, route);
  if (redressResult != RouterResultCode::NoError)
    return redressResult;

  LOG(LINFO, ("Route length:", route.GetTotalDistanceMeters(), "meters. ETA:",
      route.GetTotalTimeSec(), "seconds."));

  m_lastRoute = make_unique<SegmentedRoute>(checkpoints.GetStart(), checkpoints.GetFinish(),
                                            route.GetSubroutes());
  for (Segment const & segment : segments)
    m_lastRoute->AddStep(segment, mercator::FromLatLon(starter->GetPoint(segment, true /* front */)));

  m_lastFakeEdges = make_unique<FakeEdgesContainer>(move(*starter));

  return RouterResultCode::NoError;
}

vector<Segment> ProcessJoints(vector<JointSegment> const & jointsPath,
                              IndexGraphStarterJoints<IndexGraphStarter> & jointStarter)
{
  CHECK(!jointsPath.empty(), ());

  vector<Segment> path;
  for (auto const & joint : jointsPath)
  {
    vector<Segment> jointPath = jointStarter.ReconstructJoint(joint);
    if (jointPath.empty())
      continue;

    if (path.empty())
    {
      path = move(jointPath);
      continue;
    }

    path.insert(path.end(),
                path.back() == jointPath.front() ? jointPath.begin() + 1 : jointPath.begin(),
                jointPath.end());
  }

  return path;
}

RouterResultCode IndexRouter::CalculateSubroute(Checkpoints const & checkpoints, size_t subrouteIdx,
                                                RouterDelegate const & delegate,
                                                shared_ptr<AStarProgress> const & progress,
                                                IndexGraphStarter & starter,
                                                vector<Segment> & subroute,
                                                bool guidesActive /* = false */)
{
  CHECK(progress, (checkpoints));
  subroute.clear();

  SetupAlgorithmMode(starter, guidesActive);
  LOG(LINFO, ("Routing in mode:", starter.GetGraph().GetMode()));

  base::ScopedTimerWithLog timer("Route build");
  WorldGraphMode const mode = starter.GetGraph().GetMode();
  switch (mode)
  {
  case WorldGraphMode::Joints:
    return CalculateSubrouteJointsMode(starter, delegate, progress, subroute);
  case WorldGraphMode::NoLeaps:
    return CalculateSubrouteNoLeapsMode(starter, delegate, progress, subroute);
  case WorldGraphMode::LeapsOnly:
    return CalculateSubrouteLeapsOnlyMode(checkpoints, subrouteIdx, starter, delegate, progress,
                                          subroute);
  default: CHECK(false, ("Wrong WorldGraphMode here:", mode));
  }
  UNREACHABLE();
}

RouterResultCode IndexRouter::CalculateSubrouteJointsMode(
    IndexGraphStarter & starter, RouterDelegate const & delegate,
    shared_ptr<AStarProgress> const & progress, vector<Segment> & subroute)
{
  using JointsStarter = IndexGraphStarterJoints<IndexGraphStarter>;
  JointsStarter jointStarter(starter, starter.GetStartSegment(), starter.GetFinishSegment());

  using Visitor = JunctionVisitor<JointsStarter>;
  Visitor visitor(jointStarter, delegate, kVisitPeriod, progress);

  using Vertex = JointsStarter::Vertex;
  using Edge = JointsStarter::Edge;
  using Weight = JointsStarter::Weight;

  AStarAlgorithm<Vertex, Edge, Weight>::Params<Visitor, AStarLengthChecker> params(
      jointStarter, jointStarter.GetStartJoint(), jointStarter.GetFinishJoint(),
      nullptr /* prevRoute */, delegate.GetCancellable(), move(visitor),
      AStarLengthChecker(starter));

  RoutingResult<Vertex, Weight> routingResult;
  RouterResultCode const result =
      FindPath<Vertex, Edge, Weight>(params, {} /* mwmIds */, routingResult);

  if (result != RouterResultCode::NoError)
    return result;

  subroute = ProcessJoints(routingResult.m_path, jointStarter);
  return result;
}

RouterResultCode IndexRouter::CalculateSubrouteNoLeapsMode(
    IndexGraphStarter & starter, RouterDelegate const & delegate,
    shared_ptr<AStarProgress> const & progress, vector<Segment> & subroute)
{
  using Vertex = IndexGraphStarter::Vertex;
  using Edge = IndexGraphStarter::Edge;
  using Weight = IndexGraphStarter::Weight;

  using Visitor = JunctionVisitor<IndexGraphStarter>;
  Visitor visitor(starter, delegate, kVisitPeriod, progress);

  AStarAlgorithm<Vertex, Edge, Weight>::Params<Visitor, AStarLengthChecker> params(
      starter, starter.GetStartSegment(), starter.GetFinishSegment(), nullptr /* prevRoute */,
      delegate.GetCancellable(), move(visitor), AStarLengthChecker(starter));

  RoutingResult<Vertex, Weight> routingResult;
  set<NumMwmId> const mwmIds = starter.GetMwms();
  RouterResultCode const result = FindPath<Vertex, Edge, Weight>(params, mwmIds, routingResult);

  if (result != RouterResultCode::NoError)
    return result;

  subroute = move(routingResult.m_path);
  return RouterResultCode::NoError;
}

RouterResultCode IndexRouter::CalculateSubrouteLeapsOnlyMode(
    Checkpoints const & checkpoints, size_t subrouteIdx, IndexGraphStarter & starter,
    RouterDelegate const & delegate, shared_ptr<AStarProgress> const & progress,
    vector<Segment> & subroute)
{
  LeapsGraph leapsGraph(starter, MwmHierarchyHandler(m_numMwmIds, m_countryParentNameGetterFn));

  using Vertex = LeapsGraph::Vertex;
  using Edge = LeapsGraph::Edge;
  using Weight = LeapsGraph::Weight;

  AStarSubProgress leapsProgress(mercator::ToLatLon(checkpoints.GetPoint(subrouteIdx)),
                                 mercator::ToLatLon(checkpoints.GetPoint(subrouteIdx + 1)),
                                 kLeapsStageContribution);
  progress->AppendSubProgress(leapsProgress);

  using Visitor = JunctionVisitor<LeapsGraph>;
  Visitor visitor(leapsGraph, delegate, kVisitPeriodForLeaps, progress);

  AStarAlgorithm<Vertex, Edge, Weight>::Params<Visitor, AStarLengthChecker> params(
      leapsGraph, leapsGraph.GetStartSegment(), leapsGraph.GetFinishSegment(),
      nullptr /* prevRoute */, delegate.GetCancellable(), move(visitor),
      AStarLengthChecker(starter));

  RoutingResult<Vertex, Weight> routingResult;
  RouterResultCode const result =
      FindPath<Vertex, Edge, Weight>(params, {} /* mwmIds */, routingResult);

  progress->PushAndDropLastSubProgress();

  if (result != RouterResultCode::NoError)
    return result;

  vector<Segment> subrouteWithoutPostprocessing;
  RouterResultCode const leapsResult =
      ProcessLeapsJoints(routingResult.m_path, delegate, starter.GetGraph().GetMode(), starter,
                         progress, subrouteWithoutPostprocessing);

  if (leapsResult != RouterResultCode::NoError)
    return leapsResult;

  LeapsPostProcessor leapsPostProcessor(subrouteWithoutPostprocessing, starter);
  subroute = leapsPostProcessor.GetProcessedPath();

  return RouterResultCode::NoError;
}

RouterResultCode IndexRouter::AdjustRoute(Checkpoints const & checkpoints,
                                          m2::PointD const & startDirection,
                                          RouterDelegate const & delegate, Route & route)
{
  base::Timer timer;
  TrafficStash::Guard guard(m_trafficStash);
  auto graph = MakeWorldGraph();
  graph->SetMode(WorldGraphMode::NoLeaps);

  vector<Segment> startSegments;
  m2::PointD const & pointFrom = checkpoints.GetPointFrom();
  bool bestSegmentIsAlmostCodirectional = false;
  if (!FindBestSegments(pointFrom, startDirection, true /* isOutgoing */, *graph, startSegments,
                        bestSegmentIsAlmostCodirectional))
  {
    return RouterResultCode::StartPointNotFound;
  }

  auto const & lastSubroutes = m_lastRoute->GetSubroutes();
  CHECK(!lastSubroutes.empty(), ());
  auto const & lastSubroute = m_lastRoute->GetSubroute(checkpoints.GetPassedIdx());

  auto const & steps = m_lastRoute->GetSteps();
  CHECK(!steps.empty(), ());

  FakeEnding dummy{};
  IndexGraphStarter starter(MakeFakeEnding(startSegments, pointFrom, *graph), dummy,
                            m_lastFakeEdges->GetNumFakeEdges(), bestSegmentIsAlmostCodirectional,
                            *graph);

  starter.Append(*m_lastFakeEdges);

  vector<SegmentEdge> prevEdges;
  CHECK_LESS_OR_EQUAL(lastSubroute.GetEndSegmentIdx(), steps.size(), ());
  for (size_t i = lastSubroute.GetBeginSegmentIdx(); i < lastSubroute.GetEndSegmentIdx(); ++i)
  {
    auto const & step = steps[i];
    prevEdges.emplace_back(step.GetSegment(), starter.CalcSegmentWeight(step.GetSegment(),
                           EdgeEstimator::Purpose::Weight));
  }

  using Visitor = JunctionVisitor<IndexGraphStarter>;
  Visitor visitor(starter, delegate, kVisitPeriod);

  using Vertex = IndexGraphStarter::Vertex;
  using Edge = IndexGraphStarter::Edge;
  using Weight = IndexGraphStarter::Weight;

  AStarAlgorithm<Vertex, Edge, Weight> algorithm;
  AStarAlgorithm<Vertex, Edge, Weight>::Params<Visitor, AdjustLengthChecker> params(
      starter, starter.GetStartSegment(), {} /* finalVertex */, &prevEdges,
      delegate.GetCancellable(), move(visitor), AdjustLengthChecker(starter));

  RoutingResult<Segment, RouteWeight> result;
  auto const resultCode =
      ConvertResult<Vertex, Edge, Weight>(algorithm.AdjustRoute(params, result));
  if (resultCode != RouterResultCode::NoError)
    return resultCode;

  CHECK_GREATER_OR_EQUAL(result.m_path.size(), 2, ());
  CHECK(IndexGraphStarter::IsFakeSegment(result.m_path.front()), ());
  CHECK(IndexGraphStarter::IsFakeSegment(result.m_path.back()), ());

  vector<Route::SubrouteAttrs> subroutes;
  PushPassedSubroutes(checkpoints, subroutes);

  size_t subrouteOffset = result.m_path.size();
  subroutes.emplace_back(starter.GetStartJunction().ToPointWithAltitude(),
                         starter.GetFinishJunction().ToPointWithAltitude(), 0 /* beginSegmentIdx */,
                         subrouteOffset);

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

  auto const redressResult = RedressRoute(result.m_path, delegate.GetCancellable(), starter, route);
  if (redressResult != RouterResultCode::NoError)
    return redressResult;

  LOG(LINFO, ("Adjust route, elapsed:", timer.ElapsedSeconds(), ", prev start:", checkpoints,
              ", prev route:", steps.size(), ", new route:", result.m_path.size()));

  return RouterResultCode::NoError;
}

unique_ptr<WorldGraph> IndexRouter::MakeWorldGraph()
{
  RoutingOptions routingOptions;
  if (m_vehicleType == VehicleType::Car)
  {
    routingOptions = RoutingOptions::LoadCarOptionsFromSettings();
    LOG(LINFO, ("Avoid next roads:", routingOptions));
  }

  auto crossMwmGraph = make_unique<CrossMwmGraph>(
      m_numMwmIds, m_numMwmTree, m_vehicleModelFactory,
      m_vehicleType == VehicleType::Transit ? VehicleType::Pedestrian : m_vehicleType,
      m_countryRectFn, m_dataSource);

  auto indexGraphLoader = IndexGraphLoader::Create(
      m_vehicleType == VehicleType::Transit ? VehicleType::Pedestrian : m_vehicleType,
      m_loadAltitudes, m_numMwmIds, m_vehicleModelFactory, m_estimator, m_dataSource,
      routingOptions);

  if (m_vehicleType != VehicleType::Transit)
  {
    auto graph = make_unique<SingleVehicleWorldGraph>(
        move(crossMwmGraph), move(indexGraphLoader), m_estimator,
        MwmHierarchyHandler(m_numMwmIds, m_countryParentNameGetterFn));
    graph->SetRoutingOptions(routingOptions);
    return graph;
  }

  auto transitGraphLoader = TransitGraphLoader::Create(m_dataSource, m_numMwmIds, m_estimator);
  return make_unique<TransitWorldGraph>(move(crossMwmGraph), move(indexGraphLoader),
                                        move(transitGraphLoader), m_estimator);
}

void IndexRouter::EraseIfDeadEnd(WorldGraph & worldGraph, m2::PointD const & checkpoint,
                                 vector<IRoadGraph::FullRoadInfo> & roads) const
{
  // |deadEnds| cache is necessary to minimize number of calls a time consumption IsDeadEnd() method.
  set<Segment> deadEnds;
  base::EraseIf(roads, [&deadEnds, &worldGraph, &checkpoint, this](auto const & fullRoadInfo) {
    CHECK_GREATER_OR_EQUAL(fullRoadInfo.m_roadInfo.m_junctions.size(), 2, ());
    auto const squaredDistAndIndex = m2::CalcMinSquaredDistance(fullRoadInfo.m_roadInfo.m_junctions.begin(),
                                                                fullRoadInfo.m_roadInfo.m_junctions.end(),
                                                                checkpoint);
    auto const segmentId = squaredDistAndIndex.second;

    // Note. Checking if an edge goes to a dead end is a time consumption process.
    // So the number of checked edges should be minimized as possible.
    // Below a heuristic is used. If the closest to |checkpoint| segment of a feature
    // in forward direction is a dead end all segments of the feature is considered as dead ends.
    auto const segment = GetSegmentByEdge(Edge::MakeReal(fullRoadInfo.m_featureId, true /* forward */,
                                                         segmentId,
                                                         fullRoadInfo.m_roadInfo.m_junctions[0],
                                                         fullRoadInfo.m_roadInfo.m_junctions[1]));
    return IsDeadEndCached(segment, true /* isOutgoing */, false /* useRoutingOptions */, worldGraph,
                           deadEnds);
  });
}

bool IndexRouter::IsFencedOff(m2::PointD const & point,
                              pair<Edge, geometry::PointWithAltitude> const & edgeProjection,
                              vector<IRoadGraph::FullRoadInfo> const & fences) const
{
  auto const & edge = edgeProjection.first;
  auto const & projPoint = edgeProjection.second.GetPoint();

  for (auto const & fence : fences)
  {
    auto const & featureGeom = fence.m_roadInfo.m_junctions;
    for (size_t i = 1; i < featureGeom.size(); ++i)
    {
      auto const & fencePointFrom = featureGeom[i - 1];
      auto const & fencePointTo = featureGeom[i];
      if (IsTheSameSegments(fencePointFrom.GetPoint(), fencePointTo.GetPoint(),
                            edge.GetStartPoint(), edge.GetEndPoint()))
      {
        continue;
      }

      // If two segment are connected with its ends it's also considered as an
      // intersection according to m2::SegmentsIntersect(). On the other hand
      // it's possible that |projPoint| is an end point of |edge| and |edge|
      // is connected with other edges. To prevent fencing off such edges with their
      // neighboring edges the condition !m2::IsPointOnSegment() is added.
      if (m2::SegmentsIntersect(point, projPoint, fencePointFrom.GetPoint(),
                                fencePointTo.GetPoint()) &&
                                !m2::IsPointOnSegment(projPoint, fencePointFrom.GetPoint(),
                                fencePointTo.GetPoint()))
      {
        return true;
      }
    }
  }
  return false;
}

void IndexRouter::RoadsToNearestEdges(
    m2::PointD const & point, vector<IRoadGraph::FullRoadInfo> const & roads,
    IsEdgeProjGood const & isGood, vector<pair<Edge, geometry::PointWithAltitude>> & edgeProj) const
{
  NearestEdgeFinder finder(point, isGood);
  for (auto const & road : roads)
    finder.AddInformationSource(road);

  finder.MakeResult(edgeProj, kMaxRoadCandidates);
}

Segment IndexRouter::GetSegmentByEdge(Edge const & edge) const
{
  auto const & featureId = edge.GetFeatureId();
  auto const & info = featureId.m_mwmId.GetInfo();
  CHECK(info, ());
  auto const numMwmId = m_numMwmIds->GetId(info->GetLocalFile().GetCountryFile());
  return Segment(numMwmId, edge.GetFeatureId().m_index, edge.GetSegId(), edge.IsForward());
}

bool IndexRouter::FindClosestCodirectionalEdge(
    m2::PointD const & point, m2::PointD const & direction,
    vector<pair<Edge, geometry::PointWithAltitude>> const & candidates,
    Edge & closestCodirectionalEdge) const
{
  double constexpr kInvalidDist = numeric_limits<double>::max();
  double squareDistToClosestCodirectionalEdgeM = kInvalidDist;

  BestEdgeComparator bestEdgeComparator(point, direction);
  if (!bestEdgeComparator.IsDirectionValid())
    return false;

  for (auto const & c : candidates)
  {
    auto const & edge = c.first;
    if (!bestEdgeComparator.IsAlmostCodirectional(edge))
      continue;

    double const distM = bestEdgeComparator.GetSquaredDist(edge);
    if (distM >= squareDistToClosestCodirectionalEdgeM)
      continue;

    closestCodirectionalEdge = edge;
    squareDistToClosestCodirectionalEdgeM = distM;
  }

  return squareDistToClosestCodirectionalEdgeM != kInvalidDist;
}

bool IndexRouter::FindBestSegments(m2::PointD const & checkpoint, m2::PointD const & direction,
                                   bool isOutgoing, WorldGraph & worldGraph,
                                   vector<Segment> & bestSegments,
                                   bool & bestSegmentIsAlmostCodirectional) const
{
  auto const file = platform::CountryFile(m_countryFileFn(checkpoint));

  vector<Edge> bestEdges;
  if (!FindBestEdges(checkpoint, file, direction, isOutgoing, 40.0 /* closestEdgesRadiusM */,
                     worldGraph, bestEdges, bestSegmentIsAlmostCodirectional))
  {
    if (!FindBestEdges(checkpoint, file, direction, isOutgoing, 500.0 /* closestEdgesRadiusM */,
                       worldGraph, bestEdges, bestSegmentIsAlmostCodirectional) &&
                       bestEdges.size() < kMaxRoadCandidates)
    {
      if (!FindBestEdges(checkpoint, file, direction, isOutgoing, 2000.0 /* closestEdgesRadiusM */,
                         worldGraph, bestEdges, bestSegmentIsAlmostCodirectional))
      {
        return false;
      }
    }
  }

  bestSegments.clear();
  for (auto const & edge : bestEdges)
    bestSegments.emplace_back(GetSegmentByEdge(edge));

  return true;
}

bool IndexRouter::FindBestEdges(m2::PointD const & checkpoint,
                                platform::CountryFile const & pointCountryFile,
                                m2::PointD const & direction, bool isOutgoing,
                                double closestEdgesRadiusM, WorldGraph & worldGraph,
                                vector<Edge> & bestEdges,
                                bool & bestSegmentIsAlmostCodirectional) const
{
  CHECK(m_vehicleModelFactory, ());
  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(pointCountryFile);
  if (!handle.IsAlive())
    MYTHROW(MwmIsNotAliveException, ("Can't get mwm handle for", pointCountryFile));

  auto const rect = mercator::RectByCenterXYAndSizeInMeters(checkpoint, closestEdgesRadiusM);
  auto const isGoodFeature = [this](FeatureID const & fid) {
    auto const & info = fid.m_mwmId.GetInfo();
    return m_numMwmIds->ContainsFile(info->GetLocalFile().GetCountryFile());
  };
  auto closestRoads = m_roadGraph.FindRoads(rect, isGoodFeature);

  // Removing all dead ends from |closestRoads|. Then some candidates will be taken from |closestRoads|.
  // It's necessary to remove all dead ends for all |closestRoads| before IsFencedOff().
  // If to remove all fenced off by other features from |checkpoint| candidates at first,
  // only dead ends candidates may be left. And then the dead end candidates will be removed
  // as well as dead ends. It often happens near airports.
  EraseIfDeadEnd(worldGraph, checkpoint, closestRoads);

  // Sorting from the closest features to the further ones. The idea is the closer
  // a feature to a |checkpoint| the more chances that it crosses the segment
  // |checkpoint|, projections of |checkpoint| on feature edges. It confirmed with benchmarks.
  sort(closestRoads.begin(), closestRoads.end(),
       [&checkpoint](IRoadGraph::FullRoadInfo const & lhs, IRoadGraph::FullRoadInfo const & rhs) {
    CHECK(!lhs.m_roadInfo.m_junctions.empty(), ());
    return
        checkpoint.SquaredLength(lhs.m_roadInfo.m_junctions[0].GetPoint()) <
            checkpoint.SquaredLength(rhs.m_roadInfo.m_junctions[0].GetPoint());
  });

  // Note about necessity of removing dead ends twice.
  // At first, only real dead ends and roads which are not correct according to |worldGraph|
  // are removed in EraseIfDeadEnd() function. It's necessary to prepare correct road network
  // (|closestRoads|) which will be used in IsFencedOff() method later and |closestRoads|
  // should contain all roads independently of routing options to prevent crossing roads
  // which are switched off in RoutingOptions.
  // Then in |IsDeadEndCached(..., true /* useRoutingOptions */, ...)| below we ignore
  // candidates if it's a dead end taking into acount routing options. We ignore candidates as well
  // if they don't match RoutingOptions.
  set<Segment> deadEnds;
  auto const isGood = [&](pair<Edge, geometry::PointWithAltitude> const & edgeProj) {
    auto const segment = GetSegmentByEdge(edgeProj.first);
    if (IsDeadEndCached(segment, isOutgoing, true /* useRoutingOptions */,  worldGraph, deadEnds))
      return false;

    // Removing all candidates which are fenced off by the road graph (|closestRoads|) from |checkpoint|.
    return !IsFencedOff(checkpoint, edgeProj, closestRoads);
  };

  // Getting closest edges from |closestRoads| if they are correct according to isGood() function.
  vector<pair<Edge, geometry::PointWithAltitude>> candidates;
  RoadsToNearestEdges(checkpoint, closestRoads, isGood, candidates);

  if (candidates.empty())
    return false;

  // Looking for the closest codirectional edge. If it's not found add all good candidates.
  Edge closestCodirectionalEdge;
  BestEdgeComparator bestEdgeComparator(checkpoint, direction);
  bestSegmentIsAlmostCodirectional =
      FindClosestCodirectionalEdge(checkpoint, direction, candidates, closestCodirectionalEdge);

  if (bestSegmentIsAlmostCodirectional)
  {
    bestEdges = {closestCodirectionalEdge};
  }
  else
  {
    bestEdges.clear();
    for (auto const & c : candidates)
      bestEdges.emplace_back(c.first);
  }

  return true;
}

RouterResultCode IndexRouter::ProcessLeapsJoints(vector<Segment> const & input,
                                                 RouterDelegate const & delegate,
                                                 WorldGraphMode prevMode,
                                                 IndexGraphStarter & starter,
                                                 shared_ptr<AStarProgress> const & progress,
                                                 vector<Segment> & output)
{
  CHECK_EQUAL(prevMode, WorldGraphMode::LeapsOnly, ());

  CHECK(progress, ());
  SCOPE_GUARD(progressGuard, [&progress]() {
    progress->PushAndDropLastSubProgress();
  });

  progress->AppendSubProgress(AStarSubProgress(1.0 - kLeapsStageContribution));

  CHECK_GREATER_OR_EQUAL(input.size(), 4,
                         ("Route in LeapsOnly mode must have at least start and finish leaps."));

  LOG(LINFO, ("Start process leaps with Joints."));
  WorldGraph & worldGraph = starter.GetGraph();

  // For all leaps except the first leap which connects start to mwm exit in LeapsOnly mode we need
  // to drop first segment of the leap because we already have its twin from the previous mwm.
  bool dropFirstSegment = false;

  // While route building in LeapOnly mode we estimate start to mwm exit distance. This estimation may
  // be incorrect and it causes appearance of unneeded loops: route goes from start to wrongly selected mwm exit,
  // then we have small unneeded leap in other mwm which returns us to start mwm, then we have leap in start mwm
  // to the correct start mwm exit and then we have normal route.
  // |input| mwm ids for such case look like
  // { fake, startId, otherId, otherId, startId, startId, .. pairs of ids for other leaps .. , finishId, fake}.
  // To avoid this behavior we collapse all leaps from start to  last occurrence of startId to one leap and
  // use WorldGraph with NoLeaps mode to proccess these leap. Unlike SingleMwm mode used to process ordinary leaps
  // NoLeaps allows to use all mwms so if we really need to visit other mwm we will.
  auto const firstMwmId = input[1].GetMwmId();
  auto const startLeapEndReverseIt = find_if(input.rbegin() + 2, input.rend(),
                                             [firstMwmId](Segment const & s) { return s.GetMwmId() == firstMwmId; });
  auto const startLeapEndIt = startLeapEndReverseIt.base() - 1;
  auto const startLeapEnd = static_cast<size_t>(distance(input.begin(), startLeapEndIt));

  // The last leap processed the same way. See the comment above.
  auto const lastMwmId = input[input.size() - 2].GetMwmId();
  auto const finishLeapStartIt = find_if(startLeapEndIt, input.end(),
                                         [lastMwmId](Segment const & s) { return s.GetMwmId() == lastMwmId; });
  auto const finishLeapStart = static_cast<size_t>(distance(input.begin(), finishLeapStartIt));

  auto fillMwmIds = [&](size_t start, size_t end, set<NumMwmId> & mwmIds)
  {
    CHECK_LESS(start, input.size(), ());
    CHECK_LESS(end, input.size(), ());
    mwmIds.clear();

    if (start == startLeapEnd)
      mwmIds = starter.GetStartMwms();

    if (end == finishLeapStart)
      mwmIds = starter.GetFinishMwms();

    for (size_t i = start; i <= end; ++i)
    {
      if (input[i].GetMwmId() != kFakeNumMwmId)
        mwmIds.insert(input[i].GetMwmId());
    }
  };

  set<NumMwmId> mwmIds;
  IndexGraphStarterJoints<IndexGraphStarter> jointStarter(starter);
  size_t maxStart = 0;

  auto const runAStarAlgorithm = [&](size_t start, size_t end, WorldGraphMode mode,
                                     RoutingResult<JointSegment, RouteWeight> & routingResult)
  {
    // Clear previous loaded graphs to not spend too much memory at one time.
    worldGraph.ClearCachedGraphs();

    // Clear previous info about route.
    routingResult.Clear();
    jointStarter.Reset();

    worldGraph.SetMode(mode);
    jointStarter.Init(input[start], input[end]);

    fillMwmIds(start, end, mwmIds);

    using JointsStarter = IndexGraphStarterJoints<IndexGraphStarter>;

    using Vertex = JointsStarter::Vertex;
    using Edge = JointsStarter::Edge;
    using Weight = JointsStarter::Weight;

    maxStart = max(maxStart, start);
    auto const contribCoef = static_cast<double>(end - maxStart + 1) / (input.size());
    auto const startPoint = starter.GetPoint(input[start], true /* front */);
    auto const endPoint = starter.GetPoint(input[end], true /* front */);
    progress->AppendSubProgress({startPoint, endPoint, contribCoef});

    RouterResultCode resultCode = RouterResultCode::NoError;
    SCOPE_GUARD(progressGuard, [&]() {
      if (resultCode == RouterResultCode::NoError)
        progress->PushAndDropLastSubProgress();
      else
        progress->DropLastSubProgress();
    });

    using Visitor = JunctionVisitor<JointsStarter>;
    Visitor visitor(jointStarter, delegate, kVisitPeriod, progress);

    AStarAlgorithm<Vertex, Edge, Weight>::Params<Visitor, AStarLengthChecker> params(
        jointStarter, jointStarter.GetStartJoint(), jointStarter.GetFinishJoint(),
        nullptr /* prevRoute */, delegate.GetCancellable(), move(visitor),
        AStarLengthChecker(starter));

    resultCode = FindPath<Vertex, Edge, Weight>(params, mwmIds, routingResult);
    return resultCode;
  };

  deque<vector<Segment>> paths;
  size_t prevStart = numeric_limits<size_t>::max();
  auto const tryBuildRoute = [&](size_t start, size_t end, WorldGraphMode mode,
                                 RoutingResult<JointSegment, RouteWeight> & routingResult)
  {
    RouterResultCode const result = runAStarAlgorithm(start, end, mode, routingResult);
    if (result == RouterResultCode::NoError)
    {
      vector<Segment> subroute = ProcessJoints(routingResult.m_path, jointStarter);

      CHECK(!subroute.empty(), ());
      if (start == prevStart && !paths.empty())
        paths.pop_back();

      ASSERT(!subroute.empty(), ());
      paths.emplace_back(vector<Segment>(dropFirstSegment ? subroute.cbegin() + 1
                                                          : subroute.cbegin(), subroute.cend()));

      dropFirstSegment = true;
      prevStart = start;
      return true;
    }

    LOG(LINFO, ("Can not find path",
      "from:", starter.GetPoint(input[start], input[start].IsForward()),
      "to:", starter.GetPoint(input[end], input[end].IsForward())));

    return false;
  };

  size_t lastPrev = 0;
  size_t prev = 0;
  size_t next = 0;
  RoutingResult<JointSegment, RouteWeight> routingResult;

  for (size_t i = startLeapEnd; i <= finishLeapStart; ++i)
  {
    if (i == startLeapEnd)
    {
      prev = 0;
      next = i;
    }
    else if (i == finishLeapStart)
    {
      prev = i;
      next = input.size() - 1;
    }
    else
    {
      prev = i;
      next = i + 1;
    }

    if (!tryBuildRoute(prev, next, WorldGraphMode::JointSingleMwm, routingResult))
    {
      auto const prevPoint = starter.GetPoint(input[next], true);
      // |next + 1| - is the twin of |next|
      // |next + 2| - is the next exit.
      while (next + 2 < finishLeapStart && next != finishLeapStart)
      {
        auto const point = starter.GetPoint(input[next + 2], true);
        double const distBetweenExistsMeters = ms::DistanceOnEarth(point, prevPoint);

        static double constexpr kMinDistBetweenExitsM = 100000;  // 100 km
        if (distBetweenExistsMeters > kMinDistBetweenExitsM)
          break;

        LOG(LINFO, ("Exit:", point,
                    "too close(", distBetweenExistsMeters / 1000, "km ), try get next."));
        next += 2;
      }

      if (next + 2 > finishLeapStart || next == finishLeapStart)
        next = input.size() - 1;
      else
        next += 2;

      if (!tryBuildRoute(prev, next, WorldGraphMode::Joints, routingResult))
      {
        // Already in start
        if (prev == 0)
          return RouterResultCode::RouteNotFound;

        prev = lastPrev;
        if (prev == 0)
          dropFirstSegment = false;

        CHECK_GREATER_OR_EQUAL(prev, 0, ());
        if (!tryBuildRoute(prev, next, WorldGraphMode::Joints, routingResult))
          return RouterResultCode::RouteNotFound;
      }
    }

    lastPrev = prev;
    i = next;
  }

  while (!paths.empty())
  {
    using Iterator = vector<Segment>::iterator;
    output.insert(output.end(),
                  move_iterator<Iterator>(paths.front().begin()),
                  move_iterator<Iterator>(paths.front().end()));
    paths.pop_front();
  }

  return RouterResultCode::NoError;
}

RouterResultCode IndexRouter::RedressRoute(vector<Segment> const & segments,
                                           base::Cancellable const & cancellable,
                                           IndexGraphStarter & starter, Route & route) const
{
  CHECK(!segments.empty(), ());
  vector<geometry::PointWithAltitude> junctions;
  size_t const numPoints = IndexGraphStarter::GetRouteNumPoints(segments);
  junctions.reserve(numPoints);

  for (size_t i = 0; i < numPoints; ++i)
    junctions.emplace_back(starter.GetRouteJunction(segments, i).ToPointWithAltitude());

  IndexRoadGraph roadGraph(m_numMwmIds, starter, segments, junctions, m_dataSource);
  starter.GetGraph().SetMode(WorldGraphMode::NoLeaps);

  Route::TTimes times;
  times.reserve(segments.size());

  // Time at zero route point.
  times.emplace_back(static_cast<uint32_t>(0), 0.0);

  // Time at first route point - weight of first segment.
  double time = starter.CalculateETAWithoutPenalty(segments.front());
  times.emplace_back(static_cast<uint32_t>(1), time);

  for (size_t i = 1; i < segments.size(); ++i)
  {
    time += starter.CalculateETA(segments[i - 1], segments[i]);
    times.emplace_back(static_cast<uint32_t>(i + 1), time);
  }

  CHECK(m_directionsEngine, ());

  m_directionsEngine->SetVehicleType(m_vehicleType);
  ReconstructRoute(*m_directionsEngine, roadGraph, m_trafficStash, cancellable, junctions,
                   move(times), route);

  CHECK(m_numMwmIds, ());
  auto & worldGraph = starter.GetGraph();
  for (auto & routeSegment : route.GetRouteSegments())
  {
    routeSegment.SetTransitInfo(worldGraph.GetTransitInfo(routeSegment.GetSegment()));

    auto & segment = routeSegment.GetSegment();
    // Removing speed cameras from the route with method AreSpeedCamerasProhibited(...)
    // at runtime is necessary for maps from Jan 2019 with speed cameras where it's prohibited
    // to use them.
    if (m_vehicleType == VehicleType::Car)
    {
      routeSegment.SetRoadTypes(starter.GetRoutingOptions(segment));
      if (segment.IsRealSegment() &&
          !AreSpeedCamerasProhibited(m_numMwmIds->GetFile(segment.GetMwmId())))
      {
        routeSegment.SetSpeedCameraInfo(worldGraph.GetSpeedCamInfo(segment));
      }
    }

    if (!segment.IsRealSegment())
      starter.ConvertToReal(segment);
  }

  vector<platform::CountryFile> speedCamProhibited;
  FillSpeedCamProhibitedMwms(segments, speedCamProhibited);
  route.SetMwmsPartlyProhibitedForSpeedCams(move(speedCamProhibited));

  if (cancellable.IsCancelled())
    return RouterResultCode::Cancelled;

  if (!route.IsValid())
  {
    LOG(LERROR, ("RedressRoute failed. Segments:", segments.size()));
    return RouterResultCode::RouteNotFoundRedressRouteError;
  }

  return RouterResultCode::NoError;
}

bool IndexRouter::AreMwmsNear(IndexGraphStarter const & starter) const
{
  auto const & startMwmIds = starter.GetStartMwms();
  auto const & finishMwmIds = starter.GetFinishMwms();
  for (auto const startMwmId : startMwmIds)
  {
    m2::RectD const & rect = m_countryRectFn(m_numMwmIds->GetFile(startMwmId).GetName());
    bool found = false;
    m_numMwmTree->ForEachInRect(rect,
                                [&finishMwmIds, &found](NumMwmId id) {
                                  if (!found && finishMwmIds.count(id) > 0)
                                    found = true;
                                });
    if (found)
      return true;
  }

  return false;
}

bool IndexRouter::DoesTransitSectionExist(NumMwmId numMwmId) const
{
  CHECK(m_numMwmIds, ());
  platform::CountryFile const & file = m_numMwmIds->GetFile(numMwmId);

  MwmSet::MwmHandle handle = m_dataSource.GetMwmHandleByCountryFile(file);
  if (!handle.IsAlive())
    MYTHROW(RoutingException, ("Can't get mwm handle for", file));

  MwmValue const & mwmValue = *handle.GetValue();
  return mwmValue.m_cont.IsExist(TRANSIT_FILE_TAG);
}

RouterResultCode IndexRouter::ConvertTransitResult(set<NumMwmId> const & mwmIds,
                                                   RouterResultCode resultCode) const
{
  if (m_vehicleType != VehicleType::Transit || resultCode != RouterResultCode::RouteNotFound)
    return resultCode;

  for (auto const mwmId : mwmIds)
  {
    CHECK_NOT_EQUAL(mwmId, kFakeNumMwmId, ());
    if (!DoesTransitSectionExist(mwmId))
      return RouterResultCode::TransitRouteNotFoundNoNetwork;
  }

  return RouterResultCode::TransitRouteNotFoundTooLongPedestrian;
}

void IndexRouter::FillSpeedCamProhibitedMwms(vector<Segment> const & segments,
                                             vector<platform::CountryFile> & speedCamProhibitedMwms) const
{
  CHECK(m_numMwmIds, ());

  set<NumMwmId> mwmIds;
  for (auto const & s : segments)
    mwmIds.emplace(s.GetMwmId());

  for (auto const id : mwmIds)
  {
    if (id == kFakeNumMwmId)
      continue;

    auto const & country = m_numMwmIds->GetFile(id);
    if (AreSpeedCamerasPartlyProhibited(country))
      speedCamProhibitedMwms.emplace_back(country);
  }
}

void IndexRouter::SetupAlgorithmMode(IndexGraphStarter & starter, bool guidesActive) const
{
  // We use NoLeaps for pedestrians and bicycles with route points near to the Guides tracks
  // because it is much easier to implement. Otherwise for pedestrians and bicycles we use Joints.
  if (guidesActive)
  {
    starter.GetGraph().SetMode(WorldGraphMode::NoLeaps);
    return;
  }

  // We use leaps for cars only. Other vehicle types do not have weights in their cross-mwm sections.
  switch (m_vehicleType)
  {
  case VehicleType::Pedestrian:
  case VehicleType::Bicycle:
    starter.GetGraph().SetMode(WorldGraphMode::Joints);
    break;
  case VehicleType::Transit:
    starter.GetGraph().SetMode(WorldGraphMode::NoLeaps);
    break;
  case VehicleType::Car:
    starter.GetGraph().SetMode(AreMwmsNear(starter) ? WorldGraphMode::Joints
                                                    : WorldGraphMode::LeapsOnly);
    break;
  case VehicleType::Count:
    CHECK(false, ("Unknown vehicle type:", m_vehicleType));
    break;
  }
}
}  // namespace routing
