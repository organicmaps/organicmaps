#include "routing/car_router.hpp"

#include "routing/cross_mwm_router.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/online_cross_fetcher.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_helpers.hpp"
#include "routing/osrm_path_segment_factory.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/turns_generator.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance.hpp"
#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "indexer/feature_altitude.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scales.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/string.hpp"

#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"
#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"
#include "3party/osrm/osrm-backend/descriptors/description_factory.hpp"

#define INTERRUPT_WHEN_CANCELLED(DELEGATE) \
  do                                       \
  {                                        \
    if (DELEGATE.IsCancelled())            \
      return Cancelled;                    \
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

using RawRouteData = InternalRouteResult;

class OSRMRoutingResult : public turns::IRoutingResult
{
public:
  // turns::IRoutingResult overrides:
  TUnpackedPathSegments const & GetSegments() const override { return m_loadedSegments; }
  void GetPossibleTurns(TNodeId node, m2::PointD const & ingoingPoint,
                        m2::PointD const & junctionPoint, size_t & ingoingCount,
                        turns::TurnCandidates & outgoingTurns) const override
  {
    double const kReadCrossEpsilon = 1.0E-4;

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
      if (!loader.GetFeatureByIndex(seg.m_fid, ft))
        continue;

      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

      m2::PointD const outgoingPoint = ft.GetPoint(
          seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1 : seg.m_pointStart - 1);
      ASSERT_LESS(MercatorBounds::DistanceOnEarth(junctionPoint, ft.GetPoint(seg.m_pointStart)),
                  turns::kFeaturesNearTurnMeters, ());

      outgoingTurns.isCandidatesAngleValid = true;
      double const a =
          my::RadToDeg(turns::PiMinusTwoVectorsAngle(junctionPoint, ingoingPoint, outgoingPoint));
      outgoingTurns.candidates.emplace_back(a, targetNode, ftypes::GetHighwayClass(ft));
    }

    sort(outgoingTurns.candidates.begin(), outgoingTurns.candidates.end(),
         [](turns::TurnCandidate const & t1, turns::TurnCandidate const & t2) {
           return t1.angle < t2.angle;
         });
  }

  double GetPathLength() const override { return m_rawResult.shortestPathLength; }
  Junction GetStartPoint() const override
  {
    return Junction(m_rawResult.sourceEdge.segmentPoint, feature::kDefaultAltitudeMeters);
  }

  Junction GetEndPoint() const override
  {
    return Junction(m_rawResult.targetEdge.segmentPoint, feature::kDefaultAltitudeMeters);
  }

  OSRMRoutingResult(Index const & index, RoutingMapping & mapping, RawRoutingResult & result)
    : m_rawResult(result), m_index(index), m_routingMapping(mapping)
  {
    for (auto const & pathSegments : m_rawResult.unpackedPathSegments)
    {
      auto numSegments = pathSegments.size();
      m_loadedSegments.resize(numSegments);
      for (size_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex)
      {
        bool isStartNode = (segmentIndex == 0);
        bool isEndNode = (segmentIndex == numSegments - 1);
        if (isStartNode || isEndNode)
        {
          OsrmPathSegmentFactory(m_routingMapping, m_index, pathSegments[segmentIndex],
                                 m_rawResult.sourceEdge, m_rawResult.targetEdge, isStartNode,
                                 isEndNode, m_loadedSegments[segmentIndex]);
        }
        else
        {
          OsrmPathSegmentFactory(m_routingMapping, m_index, pathSegments[segmentIndex],
                                 m_loadedSegments[segmentIndex]);
        }
      }
    }
  }

private:
  TUnpackedPathSegments m_loadedSegments;
  RawRoutingResult m_rawResult;
  Index const & m_index;
  RoutingMapping & m_routingMapping;
};

IRouter::ResultCode FindSingleOsrmRoute(FeatureGraphNode const & source,
                                        FeatureGraphNode const & target,
                                        RouterDelegate const & delegate, Index const & index,
                                        TRoutingMappingPtr & mapping, Route & route)
{
  vector<Junction> geometry;
  Route::TTurns turns;
  Route::TTimes times;
  Route::TStreets streets;

  LOG(LINFO, ("OSRM route from", MercatorBounds::ToLatLon(source.segmentPoint), "to",
              MercatorBounds::ToLatLon(target.segmentPoint)));

  RawRoutingResult rawRoutingResult;
  if (!FindSingleRoute(source, target, mapping->m_dataFacade, rawRoutingResult))
    return routing::IRouter::RouteNotFound;

  OSRMRoutingResult routingResult(index, *mapping, rawRoutingResult);
  routing::IRouter::ResultCode const result =
      MakeTurnAnnotation(routingResult, delegate, geometry, turns, times, streets);
  if (result != routing::IRouter::NoError)
  {
    LOG(LWARNING, ("Can't load road path data from disk for", mapping->GetCountryName(),
                   ". Result code =", result));
    return result;
  }

  route.SetTurnInstructions(move(turns));
  route.SetSectionTimes(move(times));
  route.SetStreetNames(move(streets));

  vector<m2::PointD> mwmPoints;
  JunctionsToPoints(geometry, mwmPoints);
  route.SetGeometry(mwmPoints.cbegin(), mwmPoints.cend());

  return routing::IRouter::NoError;
}
}  //  namespace

// static
bool CarRouter::CheckRoutingAbility(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                                    TCountryFileFn const & countryFileFn, Index & index)
{
  RoutingIndexManager manager(countryFileFn, index);
  return manager.GetMappingByPoint(startPoint)->IsValid() &&
         manager.GetMappingByPoint(finalPoint)->IsValid();
}

CarRouter::CarRouter(Index & index, TCountryFileFn const & countryFileFn,
                     unique_ptr<AStarRouter> localRouter)
  : m_index(index), m_indexManager(countryFileFn, index), m_router(move(localRouter))
{
}

string CarRouter::GetName() const { return "mixed-car"; }

void CarRouter::ClearState()
{
  m_cachedTargets.clear();
  m_cachedTargetPoint = m2::PointD::Zero();
  m_indexManager.Clear();
}

bool CarRouter::FindRouteMSMT(TFeatureGraphNodeVec const & sources,
                              TFeatureGraphNodeVec const & targets, RouterDelegate const & delegate,
                              TRoutingMappingPtr & mapping, Route & route)
{
  ASSERT(mapping, ());

  Route emptyRoute(GetName());
  route.Swap(emptyRoute);
  /// @todo (ldargunov) make more complex nearest edge turnaround
  for (auto const & targetEdge : targets)
  {
    for (auto const & sourceEdge : sources)
    {
      if (FindSingleRouteDispatcher(sourceEdge, targetEdge, delegate, mapping, route) == NoError)
        return true;
    }
  }
  return false;
}

void FindGraphNodeOffsets(uint32_t const nodeId, m2::PointD const & point, Index const * pIndex,
                          TRoutingMappingPtr & mapping, FeatureGraphNode & graphNode)
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
    if (!loader.GetFeatureByIndex(s.m_fid, ft))
      continue;

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
CarRouter::ResultCode CarRouter::MakeRouteFromCrossesPath(TCheckedPath const & path,
                                                          RouterDelegate const & delegate,
                                                          Route & route)
{
  Route emptyRoute(GetName());
  route.Swap(emptyRoute);

  for (RoutePathCross cross : path)
  {
    ASSERT_EQUAL(cross.startNode.mwmId, cross.finalNode.mwmId, ());
    TRoutingMappingPtr mwmMapping = m_indexManager.GetMappingById(cross.startNode.mwmId);
    ASSERT(mwmMapping->IsValid(), ());
    MappingGuard mwmMappingGuard(mwmMapping);
    UNUSED_VALUE(mwmMappingGuard);
    CalculatePhantomNodeForCross(mwmMapping, cross.startNode, &m_index, true /* forward */);
    CalculatePhantomNodeForCross(mwmMapping, cross.finalNode, &m_index, false /* forward */);
    IRouter::ResultCode const result =
        FindSingleRouteDispatcher(cross.startNode, cross.finalNode, delegate, mwmMapping, route);
    if (result != NoError)
      return result;
  }

  return NoError;
}

CarRouter::ResultCode CarRouter::CalculateRoute(m2::PointD const & startPoint,
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
    ResultCode const code = FindPhantomNodes(startPoint, startDirection, startTask,
                                             kMaxNodeCandidatesCount, startMapping);
    if (code != NoError)
      return code;
  }
  {
    if (finalPoint != m_cachedTargetPoint)
    {
      ResultCode const code = FindPhantomNodes(finalPoint, m2::PointD::Zero(), m_cachedTargets,
                                               kMaxNodeCandidatesCount, targetMapping);
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
  double crossDistanceM = 0;
  TCheckedPath finalPath;

  // Manually load facade to avoid unmaping files we routing on.
  startMapping->LoadFacade();

  // 4.1 Single mwm case
  if (startMapping->GetMwmId() == targetMapping->GetMwmId())
  {
    LOG(LINFO, ("Single mwm routing case"));
    m_indexManager.ForEachMapping([](pair<string, TRoutingMappingPtr> const & indexPair) {
      indexPair.second->FreeCrossContext();
    });
    ResultCode crossCode = CalculateCrossMwmPath(startTask, m_cachedTargets, m_indexManager,
                                                 crossDistanceM, delegate, finalPath);
    LOG(LINFO, ("Found cross path in", timer.ElapsedNano(), "ns."));
    if (!FindRouteMSMT(startTask, m_cachedTargets, delegate, startMapping, route))
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

    if (crossCode == NoError && crossDistanceM < route.GetTotalDistanceMeters())
    {
      LOG(LINFO, ("Cross mwm path is shorter than single mwm path. Cross distance:",
                  crossDistanceM, "single distance:", route.GetTotalDistanceMeters()));
      auto code = MakeRouteFromCrossesPath(finalPath, delegate, route);
      LOG(LINFO, ("Made final route in", timer.ElapsedNano(), "ns."));
      timer.Reset();
      return code;
    }

    INTERRUPT_WHEN_CANCELLED(delegate);
    delegate.OnProgress(kPathFoundProgress);

    return NoError;
  }
  else  // 4.2 Multiple mwm case
  {
    LOG(LINFO, ("Multiple mwm routing case"));
    ResultCode code = CalculateCrossMwmPath(startTask, m_cachedTargets, m_indexManager,
                                            crossDistanceM, delegate, finalPath);
    timer.Reset();
    INTERRUPT_WHEN_CANCELLED(delegate);
    delegate.OnProgress(kCrossPathFoundProgress);

    // 5. Make generate answer
    if (code == NoError)
    {
      auto code = MakeRouteFromCrossesPath(finalPath, delegate, route);
      // Manually free all cross context allocations before geometry unpacking.
      m_indexManager.ForEachMapping([](pair<string, TRoutingMappingPtr> const & indexPair) {
        indexPair.second->FreeCrossContext();
      });
      LOG(LINFO, ("Made final route in", timer.ElapsedNano(), "ns."));
      timer.Reset();
      return code;
    }
    return CarRouter::RouteNotFound;
  }
}

IRouter::ResultCode CarRouter::FindPhantomNodes(m2::PointD const & point,
                                                m2::PointD const & direction,
                                                TFeatureGraphNodeVec & res, size_t maxCount,
                                                TRoutingMappingPtr const & mapping)
{
  ASSERT(mapping, ());
  helpers::Point2PhantomNode getter(*mapping, m_index, direction);
  getter.SetPoint(point);

  m_index.ForEachInRectForMWM(getter, MercatorBounds::RectByCenterXYAndSizeInMeters(
                                          point, kFeatureFindingRectSideRadiusMeters),
                              scales::GetUpperScale(), mapping->GetMwmId());

  if (!getter.HasCandidates())
    return RouteNotFound;

  getter.MakeResult(res, maxCount);
  return NoError;
}

bool CarRouter::DoesEdgeIndexExist(Index::MwmId const & mwmId)
{
  MwmSet::MwmHandle const handle = m_index.GetMwmHandleById(mwmId);
  if (!handle.IsAlive())
  {
    ASSERT(false, ("Mwm handle is not alive."));
    return false;
  }

  MwmValue const * value = handle.GetValue<MwmValue>();
  CHECK(value, ());
  if (value->GetHeader().GetFormat() < version::Format::v8)
    return false;

  if (!value->m_cont.IsExist(ROUTING_FILE_TAG))
    return false;

  return true;
}

IRouter::ResultCode CarRouter::FindSingleRouteDispatcher(FeatureGraphNode const & source,
                                                         FeatureGraphNode const & target,
                                                         RouterDelegate const & delegate,
                                                         TRoutingMappingPtr & mapping,
                                                         Route & route)
{
  ASSERT_EQUAL(source.mwmId, target.mwmId, ());
  IRouter::ResultCode result = IRouter::InternalError;
  Route mwmRoute(GetName());

  // @TODO It's not the best place for checking availability of edge index section in mwm.
  // Probably it's better to keep if mwm has an edge index section in mwmId.
  if (DoesEdgeIndexExist(source.mwmId))
  {
    if (!m_router)
    {
      LOG(LERROR, ("m_router is not initialized."));
      return IRouter::InternalError;
    }
    LOG(LINFO, (m_router->GetName(), "route from", MercatorBounds::ToLatLon(source.segmentPoint),
                "to", MercatorBounds::ToLatLon(target.segmentPoint)));
    result = m_router->CalculateRoute(source.mwmId, source.segmentPoint,
                                      m2::PointD(0, 0) /* direction */, target.segmentPoint,
                                      delegate, mwmRoute);
  }
  else
  {
    // OSRM Routing
    // @TODO This branch is implemented to support old maps with osrm section. When osrm
    // section is not supported this branch should be removed.
    result = FindSingleOsrmRoute(source, target, delegate, m_index, mapping, mwmRoute);
  }

  if (result == IRouter::NoError)
    route.AppendRoute(mwmRoute);

  return result;
}
}  // namespace routing
