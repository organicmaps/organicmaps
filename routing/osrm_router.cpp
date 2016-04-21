#include "routing/cross_mwm_router.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/osrm_path_segment_factory.hpp"
#include "routing/online_cross_fetcher.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_helpers.hpp"
#include "routing/osrm_router.hpp"
#include "routing/turns_generator.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "indexer/ftypes_matcher.hpp"
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
#include "std/unique_ptr.hpp"

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

double PiMinusTwoVectorsAngle(m2::PointD const & p, m2::PointD const & p1, m2::PointD const & p2)
{
  return math::pi - ang::TwoVectorsAngle(p, p1, p2);
}
} //  namespace

using RawRouteData = InternalRouteResult;

class OSRMRoutingResultGraph : public turns::IRoutingResultGraph
{
public:
  virtual vector<unique_ptr<turns::LoadedPathSegment>> const & GetSegments() const override
  {
    return m_loadedSegments;
  }
  virtual void GetPossibleTurns(TNodeId node, m2::PointD const & ingoingPoint,
                                m2::PointD const & junctionPoint,
                                size_t & ingoingCount,
                                turns::TTurnCandidates & outgoingTurns) const override
  {
    double const kReadCrossEpsilon = 1.0E-4;
    double const kFeaturesNearTurnMeters = 3.0;

    // Geting nodes by geometry.
    vector<NodeID> geomNodes;
    helpers::Point2Node p2n(m_routingMapping, geomNodes);

    m_index.ForEachInRectForMWM(
        p2n, m2::RectD(junctionPoint.x - kReadCrossEpsilon, junctionPoint.y - kReadCrossEpsilon,
                       junctionPoint.x + kReadCrossEpsilon, junctionPoint.y + kReadCrossEpsilon),
        scales::GetUpperScale(), m_routingMapping.GetMwmId());

    sort(geomNodes.begin(), geomNodes.end());
    geomNodes.erase(unique(geomNodes.begin(), geomNodes.end()), geomNodes.end());

    // Filtering virtual edges.
    vector<NodeID> adjacentNodes;
    ingoingCount = 0;
    for (EdgeID const e : m_routingMapping.m_dataFacade.GetAdjacentEdgeRange(node))
    {
      QueryEdge::EdgeData const data = m_routingMapping.m_dataFacade.GetEdgeData(e, node);
      if (data.shortcut)
        continue;
      if (data.forward)
      {
        adjacentNodes.push_back(m_routingMapping.m_dataFacade.GetTarget(e));
        ASSERT_NOT_EQUAL(m_routingMapping.m_dataFacade.GetTarget(e), SPECIAL_NODEID, ());
      }
      else
      {
        ++ingoingCount;
      }
    }

    for (NodeID const adjacentNode : geomNodes)
    {
      if (adjacentNode == node)
        continue;
      for (EdgeID const e : m_routingMapping.m_dataFacade.GetAdjacentEdgeRange(adjacentNode))
      {
        if (m_routingMapping.m_dataFacade.GetTarget(e) != node)
          continue;
        QueryEdge::EdgeData const data = m_routingMapping.m_dataFacade.GetEdgeData(e, adjacentNode);
        if (data.shortcut)
          continue;
        if (data.backward)
          adjacentNodes.push_back(adjacentNode);
        else
          ++ingoingCount;
      }
    }

    // Preparing candidates.
    for (NodeID const targetNode : adjacentNodes)
    {
      auto const range = m_routingMapping.m_segMapping.GetSegmentsRange(targetNode);
      OsrmMappingTypes::FtSeg seg;
      m_routingMapping.m_segMapping.GetSegmentByIndex(range.first, seg);
      if (!seg.IsValid())
        continue;

      FeatureType ft;
      Index::FeaturesLoaderGuard loader(m_index, m_routingMapping.GetMwmId());
      loader.GetFeatureByIndex(seg.m_fid, ft);
      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

      m2::PointD const outgoingPoint = ft.GetPoint(
          seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1 : seg.m_pointStart - 1);
      ASSERT_LESS(MercatorBounds::DistanceOnEarth(junctionPoint, ft.GetPoint(seg.m_pointStart)),
                  kFeaturesNearTurnMeters, ());

      double const a =
          my::RadToDeg(PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
      outgoingTurns.emplace_back(a, targetNode, ftypes::GetHighwayClass(ft));
    }

    sort(outgoingTurns.begin(), outgoingTurns.end(),
         [](turns::TurnCandidate const & t1, turns::TurnCandidate const & t2)
         {
           return t1.angle < t2.angle;
         });
  }

  virtual double GetShortestPathLength() const override { return m_rawResult.shortestPathLength; }
  virtual m2::PointD const & GetStartPoint() const override
  {
    return m_rawResult.sourceEdge.segmentPoint;
  }
  virtual m2::PointD const & GetEndPoint() const override
  {
    return m_rawResult.targetEdge.segmentPoint;
  }

  OSRMRoutingResultGraph(Index const & index, RoutingMapping & mapping, RawRoutingResult & result)
    : m_rawResult(result), m_index(index), m_routingMapping(mapping)
  {
    for (auto const & pathSegments : m_rawResult.unpackedPathSegments)
    {
      auto numSegments = pathSegments.size();
      m_loadedSegments.reserve(numSegments);
      for (size_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
      {
        bool isStartNode = (segmentIndex == 0);
        bool isEndNode = (segmentIndex == numSegments - 1);
        if (isStartNode || isEndNode)
        {
          m_loadedSegments.push_back(turns::LoadedPathSegmentFactory(m_routingMapping, m_index,
                                                                     pathSegments[segmentIndex], m_rawResult.sourceEdge,
                                                                     m_rawResult.targetEdge, isStartNode, isEndNode));
        }
        else
        {
          m_loadedSegments.push_back(turns::LoadedPathSegmentFactory(m_routingMapping, m_index,
                                                                     pathSegments[segmentIndex]));
        }
      }
    }
  }

  ~OSRMRoutingResultGraph() {}
private:
  vector<unique_ptr<turns::LoadedPathSegment>> m_loadedSegments;
  RawRoutingResult m_rawResult;
  Index const & m_index;
  RoutingMapping & m_routingMapping;
};

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
    size_t start_idx = min(s.m_pointStart, s.m_pointEnd);
    size_t stop_idx = max(s.m_pointStart, s.m_pointEnd);
    helpers::Point2PhantomNode::FindNearestSegment(ft, point, mappedSeg, start_idx, stop_idx);

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
  Route::TTurns turnsDir;
  Route::TTimes times;
  Route::TStreets streets;
  vector<m2::PointD> points;
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

    if (!points.empty())
    {
      // Remove road end point and turn instruction.
      points.pop_back();
      turnsDir.pop_back();
      times.pop_back();
      // Streets might not point to the last point of the path.
    }

    // Get annotated route.
    Route::TTurns mwmTurnsDir;
    Route::TTimes mwmTimes;
    Route::TStreets mwmStreets;
    vector<m2::PointD> mwmPoints;
    OSRMRoutingResultGraph resultGraph(*m_pIndex, *mwmMapping, routingResult);
    if (MakeTurnAnnotation(resultGraph, delegate, mwmPoints, mwmTurnsDir, mwmTimes, mwmStreets) != NoError)
    {
      LOG(LWARNING, ("Can't load road path data from disk for", mwmMapping->GetCountryName()));
      return RouteNotFound;
    }
    // Connect annotated route.
    auto const pSize = static_cast<uint32_t>(points.size());
    for (auto turn : mwmTurnsDir)
    {
      if (turn.m_index == 0)
        continue;
      turn.m_index += pSize;
      turnsDir.push_back(turn);
    }

    if (!mwmStreets.empty() && !streets.empty() && mwmStreets.front().second == streets.back().second)
      mwmStreets.erase(mwmStreets.begin());
    for (auto street : mwmStreets)
    {
      if (street.first == 0)
        continue;
      street.first += pSize;
      streets.push_back(street);
    }

    double const estimationTime = times.size() ? times.back().second : 0.0;
    for (auto time : mwmTimes)
    {
      if (time.first == 0)
        continue;
      time.first += pSize;
      time.second += estimationTime;
      times.push_back(time);
    }

    points.insert(points.end(), mwmPoints.begin(), mwmPoints.end());
  }

  route.SetGeometry(points.begin(), points.end());
  route.SetTurnInstructions(turnsDir);
  route.SetSectionTimes(times);
  route.SetStreetNames(streets);
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
  LOG(LINFO, ("Duration of the MWM loading", timer.ElapsedNano(), "ns."));
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

  LOG(LINFO, ("Duration of the start/stop points lookup", timer.ElapsedNano(), "ns."));
  timer.Reset();
  delegate.OnProgress(kPointsFoundProgress);

  // 4. Find route.
  RawRoutingResult routingResult;
  double crossCost = 0;
  TCheckedPath finalPath;

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
    ResultCode crossCode = CalculateCrossMwmPath(startTask, m_cachedTargets, m_indexManager, crossCost,
                                                 delegate, finalPath);
    LOG(LINFO, ("Found cross path in", timer.ElapsedNano(), "ns."));
    if (!FindRouteFromCases(startTask, m_cachedTargets, startMapping->m_dataFacade,
                            routingResult))
    {
      if (crossCode == NoError)
      {
        LOG(LINFO, ("Found only cross path."));
        auto code = MakeRouteFromCrossesPath(finalPath, delegate, route);
        LOG(LINFO, ("Made final route in", timer.ElapsedNano(), "ns."));
        return code;
      }
      return RouteNotFound;
    }
    INTERRUPT_WHEN_CANCELLED(delegate);

    if (crossCode == NoError && crossCost < routingResult.shortestPathLength)
    {
      LOG(LINFO, ("Cross mwm path shorter. Cross cost:", crossCost, "single cost:", routingResult.shortestPathLength));
      auto code = MakeRouteFromCrossesPath(finalPath, delegate, route);
      LOG(LINFO, ("Made final route in", timer.ElapsedNano(), "ns."));
      timer.Reset();
      return code;
    }

    INTERRUPT_WHEN_CANCELLED(delegate);
    delegate.OnProgress(kPathFoundProgress);

    // 5. Restore route.

    Route::TTurns turnsDir;
    Route::TTimes times;
    Route::TStreets streets;
    vector<m2::PointD> points;

    OSRMRoutingResultGraph resultGraph(*m_pIndex, *startMapping, routingResult);
    if (MakeTurnAnnotation(resultGraph, delegate, points, turnsDir, times, streets) != NoError)
    {
      LOG(LWARNING, ("Can't load road path data from disk!"));
      return RouteNotFound;
    }

    route.SetGeometry(points.begin(), points.end());
    route.SetTurnInstructions(turnsDir);
    route.SetSectionTimes(times);
    route.SetStreetNames(streets);

    return NoError;
  }
  else //4.2 Multiple mwm case
  {
    LOG(LINFO, ("Multiple mwm routing case"));
    ResultCode code = CalculateCrossMwmPath(startTask, m_cachedTargets, m_indexManager, crossCost,
                                            delegate, finalPath);
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
      LOG(LINFO, ("Made final route in", timer.ElapsedNano(), "ns."));
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
}  // namespace routing
