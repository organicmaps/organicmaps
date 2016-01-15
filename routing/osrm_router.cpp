#include "cross_mwm_router.hpp"
#include "online_cross_fetcher.hpp"
#include "osrm2feature_map.hpp"
#include "osrm_helpers.hpp"
#include "osrm_router.hpp"
#include "turns_generator.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance.hpp"
#include "geometry/distance_on_sphere.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "geometry/mercator.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"
#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"
#include "3party/osrm/osrm-backend/descriptors/description_factory.hpp"

#define INTERRUPT_WHEN_CANCELLED(DELEGATE) \
  do                               \
  {                                \
    if (DELEGATE.IsCancelled())    \
      return Cancelled;            \
  } while (false)

namespace routing
{

namespace
{
size_t constexpr kMaxNodeCandidatesCount = 10;
double constexpr kFeatureFindingRectSideRadiusMeters = 1000.0;
double constexpr kMwmLoadedProgress = 10.0f;
double constexpr kPointsFoundProgress = 15.0f;
double constexpr kCrossPathFoundProgress = 50.0f;
double constexpr kPathFoundProgress = 70.0f;
// Osrm multiples seconds to 10, so we need to divide it back.
double constexpr kOSRMWeightToSecondsMultiplier = 1./10.;
} //  namespace
// TODO (ldragunov) Switch all RawRouteData and incapsulate to own omim types.
using RawRouteData = InternalRouteResult;

// static
bool OsrmRouter::CheckRoutingAbility(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                                     TCountryFileFn const & countryFileFn, Index * index)
{
  RoutingIndexManager manager(countryFileFn, *index);
  return manager.GetMappingByPoint(startPoint)->IsValid() &&
         manager.GetMappingByPoint(finalPoint)->IsValid();
}

OsrmRouter::OsrmRouter(Index * index, TCountryFileFn const & countryFileFn)
    : m_pIndex(index), m_indexManager(countryFileFn, *index)
{
}

string OsrmRouter::GetName() const
{
  return "vehicle";
}

void OsrmRouter::ClearState()
{
  m_cachedTargets.clear();
  m_cachedTargetPoint = m2::PointD::Zero();
  m_indexManager.Clear();
}

bool OsrmRouter::FindRouteFromCases(TFeatureGraphNodeVec const & source,
                                    TFeatureGraphNodeVec const & target, TDataFacade & facade,
                                    RawRoutingResult & rawRoutingResult)
{
  /// @todo (ldargunov) make more complex nearest edge turnaround
  for (auto const & targetEdge : target)
    for (auto const & sourceEdge : source)
      if (FindSingleRoute(sourceEdge, targetEdge, facade, rawRoutingResult))
        return true;
  return false;
}

void FindGraphNodeOffsets(uint32_t const nodeId, m2::PointD const & point,
                          Index const * pIndex, TRoutingMappingPtr & mapping,
                          FeatureGraphNode & graphNode)
{
  graphNode.segmentPoint = point;

  helpers::Point2PhantomNode::Candidate best;

  auto range = mapping->m_segMapping.GetSegmentsRange(nodeId);
  for (size_t i = range.first; i < range.second; ++i)
  {
    OsrmMappingTypes::FtSeg s;
    mapping->m_segMapping.GetSegmentByIndex(i, s);
    if (!s.IsValid())
      continue;

    FeatureType ft;
    Index::FeaturesLoaderGuard loader(*pIndex, mapping->GetMwmId());
    loader.GetFeatureByIndex(s.m_fid, ft);

    helpers::Point2PhantomNode::Candidate mappedSeg;
    helpers::Point2PhantomNode::FindNearestSegment(ft, point, mappedSeg);

    OsrmMappingTypes::FtSeg seg;
    seg.m_fid = mappedSeg.m_fid;
    seg.m_pointStart = mappedSeg.m_segIdx;
    seg.m_pointEnd = mappedSeg.m_segIdx + 1;
    if (!s.IsIntersect(seg))
      continue;

    if (mappedSeg.m_dist < best.m_dist)
      best = mappedSeg;
  }

  CHECK_NOT_EQUAL(best.m_fid, kInvalidFid, ());

  graphNode.segment.m_fid = best.m_fid;
  graphNode.segment.m_pointStart = best.m_segIdx;
  graphNode.segment.m_pointEnd = best.m_segIdx + 1;
}

void CalculatePhantomNodeForCross(TRoutingMappingPtr & mapping, FeatureGraphNode & graphNode,
                         Index const * pIndex, bool forward)
{
  if (graphNode.segment.IsValid())
    return;

  uint32_t nodeId;
  if (forward)
    nodeId = graphNode.node.forward_node_id;
  else
    nodeId = graphNode.node.reverse_node_id;

  CHECK_NOT_EQUAL(nodeId, INVALID_NODE_ID, ());

  FindGraphNodeOffsets(nodeId, graphNode.segmentPoint, pIndex, mapping, graphNode);
}

// TODO (ldragunov) move this function to cross mwm router
// TODO (ldragunov) process case when the start and the finish points are placed on the same edge.
OsrmRouter::ResultCode OsrmRouter::MakeRouteFromCrossesPath(TCheckedPath const & path,
                                                            RouterDelegate const & delegate,
                                                            Route & route)
{
  Route::TTurns TurnsDir;
  Route::TTimes Times;
  vector<m2::PointD> Points;
  for (RoutePathCross cross : path)
  {
    ASSERT_EQUAL(cross.startNode.mwmId, cross.finalNode.mwmId, ());
    RawRoutingResult routingResult;
    TRoutingMappingPtr mwmMapping = m_indexManager.GetMappingById(cross.startNode.mwmId);
    ASSERT(mwmMapping->IsValid(), ());
    MappingGuard mwmMappingGuard(mwmMapping);
    UNUSED_VALUE(mwmMappingGuard);
    CalculatePhantomNodeForCross(mwmMapping, cross.startNode, m_pIndex, true /* forward */);
    CalculatePhantomNodeForCross(mwmMapping, cross.finalNode, m_pIndex, false /* forward */);
    if (!FindSingleRoute(cross.startNode, cross.finalNode, mwmMapping->m_dataFacade, routingResult))
      return RouteNotFound;

    if (!Points.empty())
    {
      // Remove road end point and turn instruction.
      Points.pop_back();
      TurnsDir.pop_back();
      Times.pop_back();
    }

    // Get annotated route.
    Route::TTurns mwmTurnsDir;
    Route::TTimes mwmTimes;
    vector<m2::PointD> mwmPoints;
    if (MakeTurnAnnotation(routingResult, mwmMapping, delegate, mwmPoints, mwmTurnsDir, mwmTimes) != NoError)
    {
      LOG(LWARNING, ("Can't load road path data from disk for", mwmMapping->GetCountryName()));
      return RouteNotFound;
    }
    // Connect annotated route.
    auto const pSize = static_cast<uint32_t>(Points.size());
    for (auto turn : mwmTurnsDir)
    {
      if (turn.m_index == 0)
        continue;
      turn.m_index += pSize;
      TurnsDir.push_back(turn);
    }

    double const estimationTime = Times.size() ? Times.back().second : 0.0;
    for (auto time : mwmTimes)
    {
      if (time.first == 0)
        continue;
      time.first += pSize;
      time.second += estimationTime;
      Times.push_back(time);
    }

    Points.insert(Points.end(), mwmPoints.begin(), mwmPoints.end());
  }

  route.SetGeometry(Points.begin(), Points.end());
  route.SetTurnInstructions(TurnsDir);
  route.SetSectionTimes(Times);
  return NoError;
}

OsrmRouter::ResultCode OsrmRouter::CalculateRoute(m2::PointD const & startPoint,
                                                  m2::PointD const & startDirection,
                                                  m2::PointD const & finalPoint,
                                                  RouterDelegate const & delegate, Route & route)
{
  my::HighResTimer timer(true);

  TRoutingMappingPtr startMapping = m_indexManager.GetMappingByPoint(startPoint);
  TRoutingMappingPtr targetMapping = m_indexManager.GetMappingByPoint(finalPoint);

  if (!startMapping->IsValid())
  {
    ResultCode const code = startMapping->GetError();
    if (code != NoError)
    {
      string const name = startMapping->GetCountryName();
      if (name.empty())
        return IRouter::ResultCode::StartPointNotFound;
      route.AddAbsentCountry(name);
      return code;
    }
    return IRouter::StartPointNotFound;
  }
  if (!targetMapping->IsValid())
  {
    ResultCode const code = targetMapping->GetError();
    if (code != NoError)
    {
      string const name = targetMapping->GetCountryName();
      if (name.empty())
        return IRouter::EndPointNotFound;
      route.AddAbsentCountry(name);
      return code;
    }
    return IRouter::EndPointNotFound;
  }

  MappingGuard startMappingGuard(startMapping);
  MappingGuard finalMappingGuard(targetMapping);
  UNUSED_VALUE(startMappingGuard);
  UNUSED_VALUE(finalMappingGuard);
  LOG(LINFO, ("Duration of the MWM loading", timer.ElapsedNano()));
  timer.Reset();

  delegate.OnProgress(kMwmLoadedProgress);

  // 3. Find start/end nodes.
  TFeatureGraphNodeVec startTask;

  {
    ResultCode const code = FindPhantomNodes(startPoint, startDirection,
                                             startTask, kMaxNodeCandidatesCount, startMapping);
    if (code != NoError)
      return code;
  }
  {
    if (finalPoint != m_cachedTargetPoint)
    {
      ResultCode const code =
          FindPhantomNodes(finalPoint, m2::PointD::Zero(),
                           m_cachedTargets, kMaxNodeCandidatesCount, targetMapping);
      if (code != NoError)
        return code;
      m_cachedTargetPoint = finalPoint;
    }
  }
  INTERRUPT_WHEN_CANCELLED(delegate);

  LOG(LINFO, ("Duration of the start/stop points lookup", timer.ElapsedNano()));
  timer.Reset();
  delegate.OnProgress(kPointsFoundProgress);

  // 4. Find route.
  RawRoutingResult routingResult;

  // Manually load facade to avoid unmaping files we routing on.
  startMapping->LoadFacade();

  // 4.1 Single mwm case
  if (startMapping->GetMwmId() == targetMapping->GetMwmId())
  {
    LOG(LINFO, ("Single mwm routing case"));
    m_indexManager.ForEachMapping([](pair<string, TRoutingMappingPtr> const & indexPair)
                                  {
                                    indexPair.second->FreeCrossContext();
                                  });
    if (!FindRouteFromCases(startTask, m_cachedTargets, startMapping->m_dataFacade,
                            routingResult))
    {
      return RouteNotFound;
    }
    INTERRUPT_WHEN_CANCELLED(delegate);
    delegate.OnProgress(kPathFoundProgress);

    // 5. Restore route.

    Route::TTurns turnsDir;
    Route::TTimes times;
    vector<m2::PointD> points;

    if (MakeTurnAnnotation(routingResult, startMapping, delegate, points, turnsDir, times) != NoError)
    {
      LOG(LWARNING, ("Can't load road path data from disk!"));
      return RouteNotFound;
    }

    route.SetGeometry(points.begin(), points.end());
    route.SetTurnInstructions(turnsDir);
    route.SetSectionTimes(times);

    return NoError;
  }
  else //4.2 Multiple mwm case
  {
    LOG(LINFO, ("Multiple mwm routing case"));
    TCheckedPath finalPath;
    ResultCode code = CalculateCrossMwmPath(startTask, m_cachedTargets, m_indexManager, delegate,
                                            finalPath);
    timer.Reset();
    INTERRUPT_WHEN_CANCELLED(delegate);
    delegate.OnProgress(kCrossPathFoundProgress);

    // 5. Make generate answer
    if (code == NoError)
    {
      auto code = MakeRouteFromCrossesPath(finalPath, delegate, route);
      // Manually free all cross context allocations before geometry unpacking.
      m_indexManager.ForEachMapping([](pair<string, TRoutingMappingPtr> const & indexPair)
                                    {
                                      indexPair.second->FreeCrossContext();
                                    });
      LOG(LINFO, ("Make final route", timer.ElapsedNano()));
      timer.Reset();
      return code;
    }
    return OsrmRouter::RouteNotFound;
  }
}

IRouter::ResultCode OsrmRouter::FindPhantomNodes(m2::PointD const & point,
                                                 m2::PointD const & direction,
                                                 TFeatureGraphNodeVec & res, size_t maxCount,
                                                 TRoutingMappingPtr const & mapping)
{
  ASSERT(mapping, ());
  helpers::Point2PhantomNode getter(*mapping, *m_pIndex, direction);
  getter.SetPoint(point);

  m_pIndex->ForEachInRectForMWM(getter, MercatorBounds::RectByCenterXYAndSizeInMeters(
                                            point, kFeatureFindingRectSideRadiusMeters),
                                scales::GetUpperScale(), mapping->GetMwmId());

  if (!getter.HasCandidates())
    return RouteNotFound;

  getter.MakeResult(res, maxCount);
  return NoError;
}

// @todo(vbykoianko) This method shall to be refactored. It shall be split into several
// methods. All the functionality shall be moved to the turns_generator unit.

// @todo(vbykoianko) For the time being MakeTurnAnnotation generates the turn annotation
// and the route polyline at the same time. It is better to generate it separately
// to be able to use the route without turn annotation.
OsrmRouter::ResultCode OsrmRouter::MakeTurnAnnotation(
    RawRoutingResult const & routingResult, TRoutingMappingPtr const & mapping,
    RouterDelegate const & delegate, vector<m2::PointD> & points, Route::TTurns & turnsDir,
    Route::TTimes & times)
{
  ASSERT(mapping, ());

  double estimatedTime = 0;

  LOG(LDEBUG, ("Shortest path length:", routingResult.shortestPathLength));

#ifdef DEBUG
  size_t lastIdx = 0;
#endif

  for (auto const & pathSegments : routingResult.unpackedPathSegments)
  {
    INTERRUPT_WHEN_CANCELLED(delegate);

    // Get all computed route coordinates.
    size_t const numSegments = pathSegments.size();

    // Construct loaded segments.
    vector<turns::LoadedPathSegment> loadedSegments;
    loadedSegments.reserve(numSegments);
    for (size_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
    {
      bool isStartNode = (segmentIndex == 0);
      bool isEndNode = (segmentIndex == numSegments - 1);
      if (isStartNode || isEndNode)
      {
        loadedSegments.emplace_back(*mapping, *m_pIndex, pathSegments[segmentIndex],
                                    routingResult.sourceEdge, routingResult.targetEdge, isStartNode,
                                    isEndNode);
      }
      else
      {
        loadedSegments.emplace_back(*mapping, *m_pIndex, pathSegments[segmentIndex]);
      }
    }

    // Annotate turns.
    size_t skipTurnSegments = 0;
    for (size_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
    {
      auto const & loadedSegment = loadedSegments[segmentIndex];

      // ETA information.
      double const nodeTimeSeconds = loadedSegment.m_weight * kOSRMWeightToSecondsMultiplier;

      // Turns information.
      if (segmentIndex > 0 && !points.empty() && skipTurnSegments == 0)
      {
        turns::TurnItem turnItem;
        turnItem.m_index = static_cast<uint32_t>(points.size() - 1);

        skipTurnSegments = CheckUTurnOnRoute(loadedSegments, segmentIndex, turnItem);

        turns::TurnInfo turnInfo(loadedSegments[segmentIndex - 1], loadedSegments[segmentIndex]);

        if (turnItem.m_turn == turns::TurnDirection::NoTurn)
          turns::GetTurnDirection(*m_pIndex, *mapping, turnInfo, turnItem);

#ifdef DEBUG
        double distMeters = 0.0;
        for (size_t k = lastIdx + 1; k < points.size(); ++k)
          distMeters += MercatorBounds::DistanceOnEarth(points[k - 1], points[k]);
        LOG(LDEBUG, ("Speed:", 3.6 * distMeters / nodeTimeSeconds, "kmph; Dist:", distMeters, "Time:",
                     nodeTimeSeconds, "s", lastIdx, "e", points.size(), "source:", turnItem.m_sourceName,
                     "target:", turnItem.m_targetName));
        lastIdx = points.size();
#endif
        times.push_back(Route::TTimeItem(points.size(), estimatedTime));

        //  Lane information.
        if (turnItem.m_turn != turns::TurnDirection::NoTurn)
        {
          turnItem.m_lanes = turnInfo.m_ingoing.m_lanes;
          turnsDir.push_back(move(turnItem));
        }
      }

      estimatedTime += nodeTimeSeconds;
      if (skipTurnSegments > 0)
        --skipTurnSegments;

      // Path geometry.
      points.insert(points.end(), loadedSegment.m_path.begin(), loadedSegment.m_path.end());
    }
  }

  // Path found. Points will be replaced by start and end edges points.
  if (points.size() == 1)
    points.push_back(points.front());

  if (points.size() < 2)
    return RouteNotFound;

  if (routingResult.sourceEdge.segment.IsValid())
    points.front() = routingResult.sourceEdge.segmentPoint;
  if (routingResult.targetEdge.segment.IsValid())
    points.back() = routingResult.targetEdge.segmentPoint;

  times.push_back(Route::TTimeItem(points.size() - 1, estimatedTime));
  if (routingResult.targetEdge.segment.IsValid())
  {
    turnsDir.emplace_back(
        turns::TurnItem(static_cast<uint32_t>(points.size()) - 1, turns::TurnDirection::ReachedYourDestination));
  }
  turns::FixupTurns(points, turnsDir);

#ifdef DEBUG
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (turns::GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-", t.m_targetName, "exit:", t.m_exitNum));
  }

  size_t last = 0;
  double lastTime = 0;
  for (Route::TTimeItem & t : times)
  {
    double dist = 0;
    for (size_t i = last + 1; i <= t.first; ++i)
      dist += MercatorBounds::DistanceOnEarth(points[i - 1], points[i]);

    double time = t.second - lastTime;

    LOG(LDEBUG, ("distance:", dist, "start:", last, "end:", t.first, "Time:", time, "Speed:", 3.6 * dist / time));
    last = t.first;
    lastTime = t.second;
  }
#endif
  LOG(LDEBUG, ("Estimated time:", estimatedTime, "s"));
  return OsrmRouter::NoError;
}
}  // namespace routing
