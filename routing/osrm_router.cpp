#include "cross_mwm_router.hpp"
#include "online_cross_fetcher.hpp"
#include "osrm_router.hpp"
#include "turns_generator.hpp"
#include "vehicle_model.hpp"

#include "platform/platform.hpp"

#include "geometry/angles.hpp"
#include "geometry/distance.hpp"
#include "geometry/distance_on_sphere.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/mercator.hpp"
#include "indexer/index.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/timer.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"
#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"
#include "3party/osrm/osrm-backend/descriptors/description_factory.hpp"

#define INTERRUPT_WHEN_CANCELLED() \
  do                               \
  {                                \
    if (IsCancelled())             \
      return Cancelled;            \
  } while (false)

namespace routing
{
size_t constexpr kMaxNodeCandidatesCount = 10;
double constexpr kFeatureFindingRectSideRadiusMeters = 1000.0;
double constexpr kTimeOverhead = 1.;
double constexpr kFeaturesNearTurnM = 3.0;

// TODO (ldragunov) Switch all RawRouteData and incapsulate to own omim types.
using RawRouteData = InternalRouteResult;

namespace
{
class AbsentCountryChecker
{
public:
  AbsentCountryChecker(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                       RoutingIndexManager & indexManager, Route & route)
      : m_route(route),
        m_indexManager(indexManager),
        m_fetcher(OSRM_ONLINE_SERVER_URL,
                  {MercatorBounds::XToLon(startPoint.x), MercatorBounds::YToLat(startPoint.y)},
                  {MercatorBounds::XToLon(finalPoint.x), MercatorBounds::YToLat(finalPoint.y)})
  {
  }

  ~AbsentCountryChecker()
  {
    vector<m2::PointD> const & points = m_fetcher.GetMwmPoints();
    for (m2::PointD const & point : points)
    {
      TRoutingMappingPtr mapping = m_indexManager.GetMappingByPoint(point);
      if (!mapping->IsValid())
      {
        LOG(LINFO, ("Online recomends to download: ", mapping->GetName()));
        m_route.AddAbsentCountry(mapping->GetName());
      }
    }
  }

private:
  Route & m_route;
  RoutingIndexManager & m_indexManager;
  OnlineCrossFetcher m_fetcher;
};

class Point2Geometry : private noncopyable
{
  m2::PointD m_p, m_p1;
  OsrmRouter::GeomTurnCandidateT & m_candidates;
public:
  Point2Geometry(m2::PointD const & p, m2::PointD const & p1,
                 OsrmRouter::GeomTurnCandidateT & candidates)
    : m_p(p), m_p1(p1), m_candidates(candidates)
  {
  }

  void operator() (FeatureType const & ft)
  {
    static CarModel const carModel;
    if (ft.GetFeatureType() != feature::GEOM_LINE || !carModel.IsRoad(ft))
      return;
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());

    for (size_t i = 0; i < count; ++i)
    {
      if (MercatorBounds::DistanceOnEarth(m_p, ft.GetPoint(i)) < kFeaturesNearTurnM)
      {
        if (i > 0)
          m_candidates.push_back(my::RadToDeg(ang::TwoVectorsAngle(m_p, m_p1, ft.GetPoint(i - 1))));
        if (i < count - 1)
          m_candidates.push_back(my::RadToDeg(ang::TwoVectorsAngle(m_p, m_p1, ft.GetPoint(i + 1))));
        return;
      }
    }
  }
};

class Point2PhantomNode : private noncopyable
{
  struct Candidate
  {
    double m_dist;
    uint32_t m_segIdx;
    uint32_t m_fid;
    m2::PointD m_point;

    Candidate() : m_dist(numeric_limits<double>::max()), m_fid(OsrmMappingTypes::FtSeg::INVALID_FID) {}
  };

  m2::PointD m_point;
  m2::PointD const m_direction;
  OsrmFtSegMapping const & m_mapping;
  buffer_vector<Candidate, 128> m_candidates;
  MwmSet::MwmId m_mwmId;
  Index const * m_pIndex;

public:
  Point2PhantomNode(OsrmFtSegMapping const & mapping, Index const * pIndex,
                    m2::PointD const & direction)
      : m_direction(direction), m_mapping(mapping), m_pIndex(pIndex)
  {
  }

  void SetPoint(m2::PointD const & pt)
  {
    m_point = pt;
  }

  bool HasCandidates() const
  {
    return !m_candidates.empty();
  }

  void operator() (FeatureType const & ft)
  {
    static CarModel const carModel;
    if (ft.GetFeatureType() != feature::GEOM_LINE || !carModel.IsRoad(ft))
      return;

    Candidate res;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());
    for (size_t i = 1; i < count; ++i)
    {
      /// @todo Probably, we need to get exact projection distance in meters.
      m2::ProjectionToSection<m2::PointD> segProj;
      segProj.SetBounds(ft.GetPoint(i - 1), ft.GetPoint(i));

      m2::PointD const pt = segProj(m_point);
      double const d = m_point.SquareLength(pt);
      if (d < res.m_dist)
      {
        res.m_dist = d;
        res.m_fid = ft.GetID().m_offset;
        res.m_segIdx = i - 1;
        res.m_point = pt;

        if (!m_mwmId.IsAlive())
          m_mwmId = ft.GetID().m_mwmId;
        ASSERT_EQUAL(m_mwmId, ft.GetID().m_mwmId, ());
      }
    }

    if (res.m_fid != OsrmMappingTypes::FtSeg::INVALID_FID)
      m_candidates.push_back(res);
  }

  double CalculateDistance(OsrmMappingTypes::FtSeg const & s) const
  {
    ASSERT_NOT_EQUAL(s.m_pointStart, s.m_pointEnd, ());

    Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmId);
    FeatureType ft;
    loader.GetFeature(s.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    double dist = 0.0;
    size_t n = max(s.m_pointEnd, s.m_pointStart);
    size_t i = min(s.m_pointStart, s.m_pointEnd) + 1;
    do
    {
      dist += MercatorBounds::DistanceOnEarth(ft.GetPoint(i - 1), ft.GetPoint(i));
      ++i;
    } while (i <= n);

    return dist;
  }

  void CalculateOffset(OsrmMappingTypes::FtSeg const & seg, m2::PointD const & segPt, NodeID & nodeId, int & offset, bool forward) const
  {
    if (nodeId == INVALID_NODE_ID)
      return;

    double distance = 0;
    auto const range = m_mapping.GetSegmentsRange(nodeId);
    OsrmMappingTypes::FtSeg s, cSeg;

    int si = forward ? range.second - 1 : range.first;
    int ei = forward ? range.first - 1 : range.second;
    int di = forward ? -1 : 1;

    for (int i = si; i != ei; i += di)
    {
      m_mapping.GetSegmentByIndex(i, s);
      if (!s.IsValid())
        continue;

      auto s1 = min(s.m_pointStart, s.m_pointEnd);
      auto e1 = max(s.m_pointEnd, s.m_pointStart);

      // seg.m_pointEnd - seg.m_pointStart == 1, so check
      // just a case, when seg is inside s
      if ((seg.m_pointStart != s1 || seg.m_pointEnd != e1) &&
          (s1 <= seg.m_pointStart && e1 >= seg.m_pointEnd))
      {
        cSeg.m_fid = s.m_fid;

        if (s.m_pointStart < s.m_pointEnd)
        {
          if (forward)
          {
            cSeg.m_pointEnd = seg.m_pointEnd;
            cSeg.m_pointStart = s.m_pointStart;
          }
          else
          {
            cSeg.m_pointStart = seg.m_pointStart;
            cSeg.m_pointEnd = s.m_pointEnd;
          }
        }
        else
        {
          if (forward)
          {
            cSeg.m_pointStart = s.m_pointEnd;
            cSeg.m_pointEnd = seg.m_pointEnd;
          }
          else
          {
            cSeg.m_pointEnd = seg.m_pointStart;
            cSeg.m_pointStart = s.m_pointStart;
          }
        }

        distance += CalculateDistance(cSeg);
        break;
      }
      else
        distance += CalculateDistance(s);
    }

    Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmId);
    FeatureType ft;
    loader.GetFeature(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    // node.m_seg always forward ordered (m_pointStart < m_pointEnd)
    distance -= MercatorBounds::DistanceOnEarth(ft.GetPoint(forward ? seg.m_pointEnd : seg.m_pointStart), segPt);

    offset = max(static_cast<int>(distance), 1);
  }

  void CalculateOffsets(FeatureGraphNode & node) const
  {
    CalculateOffset(node.segment, node.segmentPoint, node.node.forward_node_id, node.node.forward_offset, true);
    CalculateOffset(node.segment, node.segmentPoint, node.node.reverse_node_id, node.node.reverse_offset, false);

    // need to initialize weights for correct work of PhantomNode::GetForwardWeightPlusOffset
    // and PhantomNode::GetReverseWeightPlusOffset
    node.node.forward_weight = 0;
    node.node.reverse_weight = 0;
  }

  void MakeResult(TFeatureGraphNodeVec & res, size_t maxCount, string const & mwmName)
  {
    if (!m_mwmId.IsAlive())
      return;

    vector<OsrmMappingTypes::FtSeg> segments;

    segments.resize(maxCount);

    OsrmFtSegMapping::FtSegSetT segmentSet;
    sort(m_candidates.begin(), m_candidates.end(), [] (Candidate const & r1, Candidate const & r2)
    {
      return (r1.m_dist < r2.m_dist);
    });

    size_t const n = min(m_candidates.size(), maxCount);
    for (size_t j = 0; j < n; ++j)
    {
      OsrmMappingTypes::FtSeg & seg = segments[j];
      Candidate const & c = m_candidates[j];

      seg.m_fid = c.m_fid;
      seg.m_pointStart = c.m_segIdx;
      seg.m_pointEnd = c.m_segIdx + 1;

      segmentSet.insert(&seg);
    }

    OsrmFtSegMapping::OsrmNodesT nodes;
    m_mapping.GetOsrmNodes(segmentSet, nodes);

    res.clear();
    res.resize(maxCount);

    for (size_t j = 0; j < maxCount; ++j)
    {
      size_t const idx = j;

      if (!segments[idx].IsValid())
        continue;

      auto it = nodes.find(segments[idx].Store());
      if (it == nodes.end())
        continue;

      FeatureGraphNode & node = res[idx];

      if (!m_direction.IsAlmostZero())
      {
        // Filter income nodes by direction mode
        OsrmMappingTypes::FtSeg const & node_seg = segments[idx];
        FeatureType feature;
        Index::FeaturesLoaderGuard loader(*m_pIndex, m_mwmId);
        loader.GetFeature(node_seg.m_fid, feature);
        feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
        m2::PointD const featureDirection = feature.GetPoint(node_seg.m_pointEnd) - feature.GetPoint(node_seg.m_pointStart);
        bool const sameDirection = (m2::DotProduct(featureDirection, m_direction) / (featureDirection.Length() * m_direction.Length()) > 0);
        if (sameDirection)
        {
          node.node.forward_node_id = it->second.first;
          node.node.reverse_node_id = INVALID_NODE_ID;
        }
        else
        {
          node.node.forward_node_id = INVALID_NODE_ID;
          node.node.reverse_node_id = it->second.second;
        }
      }
      else
      {
        node.node.forward_node_id = it->second.first;
        node.node.reverse_node_id = it->second.second;
      }

      node.segment = segments[idx];
      node.segmentPoint = m_candidates[j].m_point;
      node.mwmName = mwmName;

      CalculateOffsets(node);
    }
    res.erase(remove_if(res.begin(), res.end(), [](FeatureGraphNode const & f)
                        {
                          return f.mwmName.empty();
                        }),
                        res.end());
  }
};

size_t GetLastSegmentPointIndex(pair<size_t, size_t> const & p)
{
  ASSERT_GREATER(p.second, 0, ());
  return p.second - 1;
}
} // namespace

OsrmRouter::OsrmRouter(Index const * index, TCountryFileFn const & fn,
                       RoutingVisualizerFn routingVisualization)
    : m_pIndex(index), m_indexManager(fn, index), m_routingVisualization(routingVisualization)
{
}

string OsrmRouter::GetName() const
{
  return "vehicle";
}

void OsrmRouter::ClearState()
{
  m_CachedTargetTask.clear();
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

// TODO (ldragunov) move this function to cross mwm router
OsrmRouter::ResultCode OsrmRouter::MakeRouteFromCrossesPath(TCheckedPath const & path,
                                                            Route & route)
{
  Route::TurnsT TurnsDir;
  Route::TimesT Times;
  vector<m2::PointD> Points;
  turns::TurnsGeomT TurnsGeom;
  for (RoutePathCross const & cross: path)
  {
    ASSERT_EQUAL(cross.startNode.mwmName, cross.finalNode.mwmName, ());
    RawRoutingResult routingResult;
    TRoutingMappingPtr mwmMapping = m_indexManager.GetMappingByName(cross.startNode.mwmName);
    ASSERT(mwmMapping->IsValid(), ());
    MappingGuard mwmMappingGuard(mwmMapping);
    UNUSED_VALUE(mwmMappingGuard);
    if (!FindSingleRoute(cross.startNode, cross.finalNode, mwmMapping->m_dataFacade, routingResult))
      return OsrmRouter::RouteNotFound;

    // Get annotated route
    Route::TurnsT mwmTurnsDir;
    Route::TimesT mwmTimes;
    vector<m2::PointD> mwmPoints;
    turns::TurnsGeomT mwmTurnsGeom;
    MakeTurnAnnotation(routingResult, mwmMapping, mwmPoints, mwmTurnsDir, mwmTimes, mwmTurnsGeom);

    // And connect it to result route
    for (auto turn : mwmTurnsDir)
    {
      turn.m_index += Points.size() - 1;
      TurnsDir.push_back(turn);
    }


    double const estimationTime = Times.size() ? Times.back().second : 0.0;
    for (auto time : mwmTimes)
    {
      time.first += Points.size() - 1;
      time.second += estimationTime;
      Times.push_back(time);
    }


    if (!Points.empty())
    {
      // We're at the end point
      Points.pop_back();
      // -1 because --mwmPoints.begin()
      const size_t psize = Points.size() - 1;
      for (auto & turnGeom : mwmTurnsGeom)
        turnGeom.m_indexInRoute += psize;
    }
    Points.insert(Points.end(), ++mwmPoints.begin(), mwmPoints.end());
    TurnsGeom.insert(TurnsGeom.end(), mwmTurnsGeom.begin(), mwmTurnsGeom.end());
  }

  route.SetGeometry(Points.begin(), Points.end());
  route.SetTurnInstructions(TurnsDir);
  route.SetSectionTimes(Times);
  route.SetTurnInstructionsGeometry(TurnsGeom);
  return OsrmRouter::NoError;
}

OsrmRouter::ResultCode OsrmRouter::CalculateRoute(m2::PointD const & startPoint,
                                                  m2::PointD const & startDirection,
                                                  m2::PointD const & finalPoint, Route & route)
{
// Experimental feature
#if defined(DEBUG)
  AbsentCountryChecker checker(startPoint, finalPoint, m_indexManager, route);
  UNUSED_VALUE(checker);
#endif
  my::HighResTimer timer(true);
  TRoutingMappingPtr startMapping = m_indexManager.GetMappingByPoint(startPoint);
  TRoutingMappingPtr targetMapping = m_indexManager.GetMappingByPoint(finalPoint);

  m_indexManager.Clear();  // TODO (Dragunov) make proper index manager cleaning

  if (!startMapping->IsValid())
  {
    route.AddAbsentCountry(startMapping->GetName());
    return startMapping->GetError();
  }
  if (!targetMapping->IsValid())
  {
    // Check if target is a neighbour
    startMapping->LoadCrossContext();
    auto out_iterators = startMapping->m_crossContext.GetOutgoingIterators();
    for (auto i = out_iterators.first; i != out_iterators.second; ++i)
      if (startMapping->m_crossContext.GetOutgoingMwmName(*i) == targetMapping->GetName())
      {
        route.AddAbsentCountry(targetMapping->GetName());
        return targetMapping->GetError();
      }
    return targetMapping->GetError();
  }

  MappingGuard startMappingGuard(startMapping);
  MappingGuard finalMappingGuard(targetMapping);
  UNUSED_VALUE(startMappingGuard);
  UNUSED_VALUE(finalMappingGuard);
  LOG(LINFO, ("Duration of the MWM loading", timer.ElapsedNano()));
  timer.Reset();

  // 3. Find start/end nodes.
  TFeatureGraphNodeVec startTask;

  {
    ResultCode const code = FindPhantomNodes(startPoint, startDirection,
                                             startTask, kMaxNodeCandidatesCount, startMapping);
    if (code != NoError)
      return code;
  }
  {
    if (finalPoint != m_CachedTargetPoint)
    {
      ResultCode const code =
          FindPhantomNodes(finalPoint, m2::PointD::Zero(),
                           m_CachedTargetTask, kMaxNodeCandidatesCount, targetMapping);
      if (code != NoError)
        return code;
      m_CachedTargetPoint = finalPoint;
    }
  }
  INTERRUPT_WHEN_CANCELLED();

  LOG(LINFO, ("Duration of the start/stop points lookup", timer.ElapsedNano()));
  timer.Reset();

  // 4. Find route.
  RawRoutingResult routingResult;

  // 4.1 Single mwm case
  if (startMapping->GetName() == targetMapping->GetName())
  {
    LOG(LINFO, ("Single mwm routing case"));
    m_indexManager.ForEachMapping([](pair<string, TRoutingMappingPtr> const & indexPair)
                                  {
                                    indexPair.second->FreeCrossContext();
                                  });
    if (!FindRouteFromCases(startTask, m_CachedTargetTask, startMapping->m_dataFacade,
                            routingResult))
    {
      return RouteNotFound;
    }
    INTERRUPT_WHEN_CANCELLED();

    // 5. Restore route.

    Route::TurnsT turnsDir;
    Route::TimesT times;
    vector<m2::PointD> points;
    turns::TurnsGeomT turnsGeom;

    MakeTurnAnnotation(routingResult, startMapping, points, turnsDir, times, turnsGeom);

    route.SetGeometry(points.begin(), points.end());
    route.SetTurnInstructions(turnsDir);
    route.SetSectionTimes(times);
    route.SetTurnInstructionsGeometry(turnsGeom);

    return NoError;
  }
  else //4.2 Multiple mwm case
  {
    LOG(LINFO, ("Multiple mwm routing case"));
    TCheckedPath finalPath;
    ResultCode code = CalculateCrossMwmPath(startTask, m_CachedTargetTask, m_indexManager,
                                            m_routingVisualization, finalPath);
    timer.Reset();

    // 5. Make generate answer
    if (code == NoError)
    {
      // Manually free all cross context allocations before geometry unpacking
      m_indexManager.ForEachMapping([](pair<string, TRoutingMappingPtr> const & indexPair)
                                    {
                                      indexPair.second->FreeCrossContext();
                                    });
      auto code = MakeRouteFromCrossesPath(finalPath, route);
      LOG(LINFO, ("Make final route", timer.ElapsedNano()));
      timer.Reset();
      return code;
    }
    return OsrmRouter::RouteNotFound;
  }
}

m2::PointD OsrmRouter::GetPointForTurnAngle(OsrmMappingTypes::FtSeg const & seg,
                                            FeatureType const & ft, m2::PointD const & turnPnt,
                                            size_t (*GetPndInd)(const size_t, const size_t, const size_t)) const
{
  const size_t maxPntsNum = 7;
  const double maxDistMeter = 300.f;
  double curDist = 0.f;
  m2::PointD pnt = turnPnt, nextPnt;

  const size_t segDist = abs(seg.m_pointEnd - seg.m_pointStart);
  ASSERT_LESS(segDist, ft.GetPointsCount(), ());
  const size_t usedFtPntNum = min(maxPntsNum, segDist);

  for (size_t i = 1; i <= usedFtPntNum; ++i)
  {
    nextPnt = ft.GetPoint(GetPndInd(seg.m_pointStart, seg.m_pointEnd, i));
    curDist += MercatorBounds::DistanceOnEarth(pnt, nextPnt);
    if (curDist > maxDistMeter)
    {
      return nextPnt;
    }
    pnt = nextPnt;
  }
  return nextPnt;
}

OsrmRouter::ResultCode OsrmRouter::MakeTurnAnnotation(RawRoutingResult const & routingResult,
                                                      TRoutingMappingPtr const & mapping,
                                                      vector<m2::PointD> & points,
                                                      Route::TurnsT & turnsDir,
                                                      Route::TimesT & times,
                                                      turns::TurnsGeomT & turnsGeom)
{
  typedef OsrmMappingTypes::FtSeg SegT;
  SegT const & segBegin = routingResult.sourceEdge.segment;
  SegT const & segEnd = routingResult.targetEdge.segment;

  double estimateTime = 0;

  LOG(LDEBUG, ("Shortest path length:", routingResult.shortestPathLength));

  //! @todo: Improve last segment time calculation
  CarModel carModel;
#ifdef _DEBUG
  size_t lastIdx = 0;
#endif
  for (auto const & segment : routingResult.unpackedPathSegments)
  {
    INTERRUPT_WHEN_CANCELLED();

    // Get all the coordinates for the computed route
    size_t const n = segment.size();
    for (size_t j = 0; j < n; ++j)
    {
      RawPathData const & path_data = segment[j];

      if (j > 0 && !points.empty())
      {
        TurnItem t;
        t.m_index = points.size() - 1;

        GetTurnDirection(segment[j - 1],
                         segment[j], mapping, t);
        if (t.m_turn != turns::TurnDirection::NoTurn)
        {
          // adding lane info
          t.m_lanes = turns::GetLanesInfo(segment[j - 1].node, *mapping,
                                          GetLastSegmentPointIndex, *m_pIndex);
          turnsDir.push_back(move(t));
        }

        // osrm multiple seconds to 10, so we need to divide it back
        double const sTime = kTimeOverhead * path_data.segmentWeight / 10.0;
#ifdef _DEBUG
        double dist = 0.0;
        for (size_t l = lastIdx + 1; l < points.size(); ++l)
          dist += MercatorBounds::DistanceOnEarth(points[l - 1], points[l]);
        LOG(LDEBUG, ("Speed:", 3.6 * dist / sTime, "kmph; Dist:", dist, "Time:", sTime, "s", lastIdx, "e", points.size()));
        lastIdx = points.size();
#endif
        estimateTime += sTime;
        times.push_back(Route::TimeItemT(points.size(), estimateTime));
      }

      buffer_vector<SegT, 8> buffer;
      mapping->m_segMapping.ForEachFtSeg(path_data.node, MakeBackInsertFunctor(buffer));

      auto FindIntersectingSeg = [&buffer] (SegT const & seg) -> size_t
      {
        ASSERT(seg.IsValid(), ());
        auto const it = find_if(buffer.begin(), buffer.end(), [&seg] (OsrmMappingTypes::FtSeg const & s)
        {
          return s.IsIntersect(seg);
        });

        ASSERT(it != buffer.end(), ());
        return distance(buffer.begin(), it);
      };

      //m_mapping.DumpSegmentByNode(path_data.node);

      //Do not put out node geometry (we do not have it)!
      size_t startK = 0, endK = buffer.size();
      if (j == 0)
      {
        if (!segBegin.IsValid())
          continue;
        startK = FindIntersectingSeg(segBegin);
      }
      if (j == n - 1)
      {
        if  (!segEnd.IsValid())
          continue;
        endK = FindIntersectingSeg(segEnd) + 1;
      }

      for (size_t k = startK; k < endK; ++k)
      {
        SegT const & seg = buffer[k];

        FeatureType ft;
        Index::FeaturesLoaderGuard loader(*m_pIndex, mapping->GetMwmId());
        loader.GetFeature(seg.m_fid, ft);
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        auto startIdx = seg.m_pointStart;
        auto endIdx = seg.m_pointEnd;
        bool const needTime = (j == 0) || (j == n - 1);

        if (j == 0 && k == startK && segBegin.IsValid())
          startIdx = (seg.m_pointEnd > seg.m_pointStart) ? segBegin.m_pointStart : segBegin.m_pointEnd;
        if (j == n - 1 && k == endK - 1 && segEnd.IsValid())
          endIdx = (seg.m_pointEnd > seg.m_pointStart) ? segEnd.m_pointEnd : segEnd.m_pointStart;

        if (seg.m_pointEnd > seg.m_pointStart)
        {
          for (auto idx = startIdx; idx <= endIdx; ++idx)
          {
            points.push_back(ft.GetPoint(idx));
            if (needTime && idx > startIdx)
              estimateTime += MercatorBounds::DistanceOnEarth(ft.GetPoint(idx - 1), ft.GetPoint(idx)) / carModel.GetSpeed(ft);
          }
        }
        else
        {
          for (auto idx = startIdx; idx > endIdx; --idx)
          {
            if (needTime)
              estimateTime += MercatorBounds::DistanceOnEarth(ft.GetPoint(idx - 1), ft.GetPoint(idx)) / carModel.GetSpeed(ft);
            points.push_back(ft.GetPoint(idx));
          }
          points.push_back(ft.GetPoint(endIdx));
        }
      }
    }
  }

  if (points.size() < 2)
    return RouteNotFound;

  if (routingResult.sourceEdge.segment.IsValid())
    points.front() = routingResult.sourceEdge.segmentPoint;
  if (routingResult.targetEdge.segment.IsValid())
    points.back() = routingResult.targetEdge.segmentPoint;

  times.push_back(Route::TimeItemT(points.size() - 1, estimateTime));
  if (routingResult.targetEdge.segment.IsValid())
    turnsDir.push_back(TurnItem(points.size() - 1, turns::TurnDirection::ReachedYourDestination));
  turns::FixupTurns(points, turnsDir);

  turns::CalculateTurnGeometry(points, turnsDir, turnsGeom);

#ifdef _DEBUG
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (turns::GetTurnString(t.m_turn), ":", t.m_index, t.m_sourceName, "-", t.m_targetName, "exit:", t.m_exitNum));
  }

  size_t last = 0;
  double lastTime = 0;
  for (Route::TimeItemT t : times)
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
  LOG(LDEBUG, ("Estimate time:", estimateTime, "s"));
  return OsrmRouter::NoError;
}

NodeID OsrmRouter::GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData, TRoutingMappingPtr const & routingMapping)
{
  ASSERT_NOT_EQUAL(src, SPECIAL_NODEID, ());
  ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());
  if (!edgeData.shortcut)
    return trg;

  ASSERT_LESS(edgeData.id, routingMapping->m_dataFacade.GetNumberOfNodes(), ());
  EdgeID edge = SPECIAL_EDGEID;
  QueryEdge::EdgeData d;
  for (EdgeID e : routingMapping->m_dataFacade.GetAdjacentEdgeRange(edgeData.id))
  {
    if (routingMapping->m_dataFacade.GetTarget(e) == src )
    {
      d = routingMapping->m_dataFacade.GetEdgeData(e, edgeData.id);
      if (d.backward)
      {
        edge = e;
        break;
      }
    }
  }

  if (edge == SPECIAL_EDGEID)
  {
    for (EdgeID e : routingMapping->m_dataFacade.GetAdjacentEdgeRange(src))
    {
      if (routingMapping->m_dataFacade.GetTarget(e) == edgeData.id)
      {
        d = routingMapping->m_dataFacade.GetEdgeData(e, src);
        if (d.forward)
        {
          edge = e;
          break;
        }
      }
    }
  }
  ASSERT_NOT_EQUAL(edge, SPECIAL_EDGEID, ());

  if (d.shortcut)
    return GetTurnTargetNode(src, edgeData.id, d, routingMapping);

  return edgeData.id;
}

void OsrmRouter::GetPossibleTurns(NodeID node, m2::PointD const & p1, m2::PointD const & p,
                                  TRoutingMappingPtr const & routingMapping,
                                  turns::TTurnCandidates & candidates)
{
  ASSERT(routingMapping.get(), ());
  for (EdgeID e : routingMapping->m_dataFacade.GetAdjacentEdgeRange(node))
  {
    QueryEdge::EdgeData const data = routingMapping->m_dataFacade.GetEdgeData(e, node);
    if (!data.forward)
      continue;

    NodeID trg = GetTurnTargetNode(node, routingMapping->m_dataFacade.GetTarget(e), data, routingMapping);
    ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());

    auto const range = routingMapping->m_segMapping.GetSegmentsRange(trg);
    OsrmMappingTypes::FtSeg seg;
    routingMapping->m_segMapping.GetSegmentByIndex(range.first, seg);
    if (!seg.IsValid())
      continue;

    FeatureType ft;
    Index::FeaturesLoaderGuard loader(*m_pIndex, routingMapping->GetMwmId());
    loader.GetFeature(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    m2::PointD const p2 = ft.GetPoint(seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1 : seg.m_pointStart - 1);
    ASSERT_LESS(MercatorBounds::DistanceOnEarth(p, ft.GetPoint(seg.m_pointStart)),
                kFeaturesNearTurnM, ());

    double const a = my::RadToDeg(ang::TwoVectorsAngle(p, p1, p2));

    candidates.emplace_back(a, trg);
  }

  sort(candidates.begin(), candidates.end(),
       [](turns::TurnCandidate const & t1, turns::TurnCandidate const & t2)
  {
    return t1.m_node < t2.m_node;
  });

  auto const last = unique(candidates.begin(), candidates.end(),
                     [](turns::TurnCandidate const & t1, turns::TurnCandidate const & t2)
  {
    return t1.m_node == t2.m_node;
  });
  candidates.erase(last, candidates.end());

  sort(candidates.begin(), candidates.end(),
       [](turns::TurnCandidate const & t1, turns::TurnCandidate const & t2)
  {
    return t1.m_angle < t2.m_angle;
  });
}

void OsrmRouter::GetTurnGeometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                                 GeomTurnCandidateT & candidates,
                                 TRoutingMappingPtr const & mapping) const
{
  ASSERT(mapping.get(), ());
  Point2Geometry getter(junctionPoint, ingoingPoint, candidates);
  m_pIndex->ForEachInRectForMWM(
      getter, MercatorBounds::RectByCenterXYAndSizeInMeters(junctionPoint, kFeaturesNearTurnM),
      scales::GetUpperScale(), mapping->GetMwmId());
}

size_t OsrmRouter::NumberOfIngoingAndOutgoingSegments(m2::PointD const & junctionPoint,
                                                      m2::PointD const & ingoingPointOneSegment,
                                                      TRoutingMappingPtr const & mapping) const
{
  ASSERT(mapping.get(), ());

  GeomTurnCandidateT geoNodes;
  GetTurnGeometry(junctionPoint, ingoingPointOneSegment, geoNodes, mapping);
  return geoNodes.size();
}

// @todo(vbykoianko) Move this method and all dependencies to turns_generator.cpp
void OsrmRouter::GetTurnDirection(RawPathData const & node1, RawPathData const & node2,
                                  TRoutingMappingPtr const & routingMapping, TurnItem & turn)
{
  ASSERT(routingMapping.get(), ());
  OsrmMappingTypes::FtSeg const seg1 =
      turns::GetSegment(node1.node, *routingMapping, GetLastSegmentPointIndex);
  OsrmMappingTypes::FtSeg const seg2 =
      turns::GetSegment(node2.node, *routingMapping, turns::GetFirstSegmentPointIndex);

  if (!seg1.IsValid() || !seg2.IsValid())
  {
    LOG(LWARNING, ("Some turns can't load geometry"));
    turn.m_turn = turns::TurnDirection::NoTurn;
    return;
  }

  FeatureType ft1, ft2;
  Index::FeaturesLoaderGuard loader1(*m_pIndex, routingMapping->GetMwmId());
  Index::FeaturesLoaderGuard loader2(*m_pIndex, routingMapping->GetMwmId());
  loader1.GetFeature(seg1.m_fid, ft1);
  loader2.GetFeature(seg2.m_fid, ft2);

  ft1.ParseGeometry(FeatureType::BEST_GEOMETRY);
  ft2.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_LESS(MercatorBounds::DistanceOnEarth(ft1.GetPoint(seg1.m_pointEnd),
                                              ft2.GetPoint(seg2.m_pointStart)),
              kFeaturesNearTurnM, ());

  m2::PointD const p = ft1.GetPoint(seg1.m_pointEnd);
  m2::PointD const p1 = GetPointForTurnAngle(seg1, ft1, p,
                    [](const size_t start, const size_t end, const size_t i)
                    {
                      return end > start ? end - i : end + i;
                    });
  m2::PointD const p2 = GetPointForTurnAngle(seg2, ft2, p,
                    [](const size_t start, const size_t end, const size_t i)
                    {
                      return end > start ? start + i : start - i;
                    });
  double const a = my::RadToDeg(ang::TwoVectorsAngle(p, p1, p2));

  m2::PointD const p1OneSeg = ft1.GetPoint(seg1.m_pointStart < seg1.m_pointEnd ? seg1.m_pointEnd - 1 : seg1.m_pointEnd + 1);
  turns::TTurnCandidates nodes;
  GetPossibleTurns(node1.node, p1OneSeg, p, routingMapping, nodes);

  turn.m_turn = turns::TurnDirection::NoTurn;
  size_t const nodesSz = nodes.size();
  bool const hasMultiTurns = (nodesSz >= 2);

  if (nodesSz == 0)
  {
    return;
  }

  if (nodes.front().m_node == node2.node)
    turn.m_turn = turns::MostRightDirection(a);
  else if (nodes.back().m_node == node2.node)
    turn.m_turn = turns::MostLeftDirection(a);
  else
    turn.m_turn = turns::IntermediateDirection(a);

  bool const isIngoingEdgeRoundabout = ftypes::IsRoundAboutChecker::Instance()(ft1);
  bool const isOutgoingEdgeRoundabout = ftypes::IsRoundAboutChecker::Instance()(ft2);

  if (isIngoingEdgeRoundabout || isOutgoingEdgeRoundabout)
  {
    turn.m_turn = turns::GetRoundaboutDirection(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout,
                                                hasMultiTurns);
    return;
  }

  turn.m_keepAnyway = (!ftypes::IsLinkChecker::Instance()(ft1)
                       && ftypes::IsLinkChecker::Instance()(ft2));

  // get names
  string name1, name2;
  {
    ft1.GetName(FeatureType::DEFAULT_LANG, turn.m_sourceName);
    ft2.GetName(FeatureType::DEFAULT_LANG, turn.m_targetName);

    search::GetStreetNameAsKey(turn.m_sourceName, name1);
    search::GetStreetNameAsKey(turn.m_targetName, name2);
  }

  ftypes::HighwayClass const highwayClass1 = ftypes::GetHighwayClass(ft1);
  ftypes::HighwayClass const highwayClass2 = ftypes::GetHighwayClass(ft2);
  if (!turn.m_keepAnyway &&
      !turns::HighwayClassFilter(highwayClass1, highwayClass2, node2.node, turn.m_turn, nodes,
                                 *routingMapping, *m_pIndex))
  {
    turn.m_turn = turns::TurnDirection::NoTurn;
    return;
  }

  bool const isGoStraightOrSlightTurn =
      turns::IsGoStraightOrSlightTurn(turns::IntermediateDirection(my::RadToDeg(ang::TwoVectorsAngle(p,p1OneSeg, p2))));
  // The code below is resposible for cases when there is only one way to leave the junction.
  // Such junction has to be kept as a turn when
  // * it's not a slight turn and it has ingoing edges (one or more);
  // * it's an entrance to a roundabout;
  if (!hasMultiTurns &&
      (isGoStraightOrSlightTurn || NumberOfIngoingAndOutgoingSegments(p, p1OneSeg, routingMapping) <= 2) &&
      !turns::CheckRoundaboutEntrance(isIngoingEdgeRoundabout, isOutgoingEdgeRoundabout))
  {
    turn.m_turn = turns::TurnDirection::NoTurn;
    return;
  }

  if (turn.m_turn == turns::TurnDirection::GoStraight)
  {
    if (!hasMultiTurns)
      turn.m_turn = turns::TurnDirection::NoTurn;

    return;
  }

  // @todo(vbykoianko) Checking if it's a uturn or not shall be moved to FindDirectionByAngle.
  if (turn.m_turn == turns::TurnDirection::NoTurn)
    turn.m_turn = turns::TurnDirection::UTurn;
}

IRouter::ResultCode OsrmRouter::FindPhantomNodes(m2::PointD const & point,
                                                 m2::PointD const & direction,
                                                 TFeatureGraphNodeVec & res, size_t maxCount,
                                                 TRoutingMappingPtr const & mapping)
{
  Point2PhantomNode getter(mapping->m_segMapping, m_pIndex, direction);
  getter.SetPoint(point);

  m_pIndex->ForEachInRectForMWM(getter, MercatorBounds::RectByCenterXYAndSizeInMeters(
                                            point, kFeatureFindingRectSideRadiusMeters),
                                scales::GetUpperScale(), mapping->GetMwmId());

  if (!getter.HasCandidates())
    return RouteNotFound;

  getter.MakeResult(res, maxCount, mapping->GetName());
  return NoError;
}

}
