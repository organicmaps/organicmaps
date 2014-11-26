#include "osrm_router.hpp"
#include "vehicle_model.hpp"

#include "../base/math.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/distance.hpp"
#include "../geometry/distance_on_sphere.hpp"

#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/index.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/mwm_version.hpp"
#include "../indexer/search_string_utils.hpp"

#include "../platform/platform.hpp"

#include "../coding/reader_wrapper.hpp"

#include "../base/logging.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/SearchEngineData.h"
#include "../3party/osrm/osrm-backend/Descriptors/DescriptionFactory.h"
#include "../3party/osrm/osrm-backend/RoutingAlgorithms/ShortestPathRouting.h"


namespace routing
{

size_t const MAX_NODE_CANDIDATES = 10;
double const FEATURE_BY_POINT_RADIUS_M = 1000.0;

namespace
{

class Point2PhantomNode : private noncopyable
{
  m2::PointD m_point;
  m2::PointD const m_direction;
  OsrmFtSegMapping const & m_mapping;

  struct Candidate
  {
    double m_dist;
    uint32_t m_segIdx;
    uint32_t m_fid;
    m2::PointD m_point;

    Candidate() : m_dist(numeric_limits<double>::max()), m_fid(OsrmFtSegMapping::FtSeg::INVALID_FID) {}
  };

  size_t m_ptIdx;
  buffer_vector<Candidate, 128> m_candidates[2];
  uint32_t m_mwmId;
  Index const * m_pIndex;

public:
  Point2PhantomNode(OsrmFtSegMapping const & mapping, Index const * pIndex, m2::PointD const & direction)
    : m_direction(direction), m_mapping(mapping), m_ptIdx(0),
      m_mwmId(numeric_limits<uint32_t>::max()), m_pIndex(pIndex)
  {
  }

  void SetPoint(m2::PointD const & pt, size_t idx)
  {
    ASSERT_LESS(idx, 2, ());
    m_point = pt;
    m_ptIdx = idx;
  }

  bool HasCandidates(uint32_t idx) const
  {
    ASSERT_LESS(idx, 2, ());
    return !m_candidates[idx].empty();
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

        if (m_mwmId == numeric_limits<uint32_t>::max())
          m_mwmId = ft.GetID().m_mwm;
        ASSERT_EQUAL(m_mwmId, ft.GetID().m_mwm, ());
      }
    }

    if (res.m_fid != OsrmFtSegMapping::FtSeg::INVALID_FID)
      m_candidates[m_ptIdx].push_back(res);
  }

  double CalculateDistance(OsrmFtSegMapping::FtSeg const & s) const
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

  void CalculateOffset(OsrmFtSegMapping::FtSeg const & seg, m2::PointD const & segPt, NodeID & nodeId, int & offset, bool forward) const
  {
    if (nodeId == INVALID_NODE_ID)
      return;

    double distance = 0;
    auto const range = m_mapping.GetSegmentsRange(nodeId);
    OsrmFtSegMapping::FtSeg s, cSeg;

    int si = forward ? range.second - 1 : range.first;
    int ei = forward ? range.first - 1 : range.second;
    int di = forward ? -1 : 1;

    for (int i = si; i != ei; i += di)
    {
      m_mapping.GetSegmentByIndex(i, s);
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

  void CalculateOffsets(OsrmRouter::FeatureGraphNode & node) const
  {
    CalculateOffset(node.m_seg, node.m_segPt, node.m_node.forward_node_id, node.m_node.forward_offset, true);
    CalculateOffset(node.m_seg, node.m_segPt, node.m_node.reverse_node_id, node.m_node.reverse_offset, false);

    // need to initialize weights for correct work of PhantomNode::GetForwardWeightPlusOffset
    // and PhantomNode::GetReverseWeightPlusOffset
    node.m_node.forward_weight = 0;
    node.m_node.reverse_weight = 0;
  }

  void MakeResult(OsrmRouter::FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId,
                  bool needFinal, volatile bool const & requestCancel)
  {
    mwmId = m_mwmId;
    if (mwmId == numeric_limits<uint32_t>::max())
      return;

    vector<OsrmFtSegMapping::FtSeg> segments;

    size_t const processPtCount = needFinal ? 2 : 1;
    segments.resize(maxCount * processPtCount);

    OsrmFtSegMapping::FtSegSetT segmentSet;
    for (size_t i = 0; i < processPtCount; ++i)
    {
      sort(m_candidates[i].begin(), m_candidates[i].end(), [] (Candidate const & r1, Candidate const & r2)
      {
        return (r1.m_dist < r2.m_dist);
      });

      size_t const n = min(m_candidates[i].size(), maxCount);
      for (size_t j = 0; j < n; ++j)
      {
        OsrmFtSegMapping::FtSeg & seg = segments[i * maxCount + j];
        Candidate const & c = m_candidates[i][j];

        seg.m_fid = c.m_fid;
        seg.m_pointStart = c.m_segIdx;
        seg.m_pointEnd = c.m_segIdx + 1;

        segmentSet.insert(&seg);
      }
    }

    OsrmFtSegMapping::OsrmNodesT nodes;
    m_mapping.GetOsrmNodes(segmentSet, nodes, requestCancel);

    res.clear();
    res.resize(maxCount * 2);

    for (size_t i = 0; i < processPtCount; ++i)
      for (size_t j = 0; j < maxCount; ++j)
      {
        size_t const idx = i * maxCount + j;

        if (!segments[idx].IsValid())
          continue;

        auto it = nodes.find(segments[idx].Store());
        if (it != nodes.end())
        {
          OsrmRouter::FeatureGraphNode & node = res[idx];

          if (!m_direction.IsAlmostZero())
          {
            // Filter income nodes by direction mode
            OsrmFtSegMapping::FtSeg const & node_seg = segments[idx];
            FeatureType feature;
            Index::FeaturesLoaderGuard loader(*m_pIndex, mwmId);
            loader.GetFeature(node_seg.m_fid, feature);
            feature.ParseGeometry(FeatureType::BEST_GEOMETRY);
            m2::PointD const featureDirection = feature.GetPoint(node_seg.m_pointEnd) - feature.GetPoint(node_seg.m_pointStart);
            bool const sameDirection = (m2::DotProduct(featureDirection, m_direction) / (featureDirection.Length() * m_direction.Length()) > 0);
            if (sameDirection)
            {
              node.m_node.forward_node_id = it->second.first;
              node.m_node.reverse_node_id = INVALID_NODE_ID;
            }
            else
            {
              node.m_node.forward_node_id = INVALID_NODE_ID;
              node.m_node.reverse_node_id = it->second.second;
            }
          }
          else
          {
            node.m_node.forward_node_id = it->second.first;
            node.m_node.reverse_node_id = it->second.second;
          }

          node.m_seg = segments[idx];
          node.m_segPt = m_candidates[i][j].m_point;

          CalculateOffsets(node);
        }
      }
  }
};

} // namespace


OsrmRouter::OsrmRouter(Index const * index, CountryFileFnT const & fn)
  : m_countryFn(fn), m_pIndex(index), m_isFinalChanged(false),
    m_requestCancel(false)
{
  m_isReadyThread.clear();
}

string OsrmRouter::GetName() const
{
  return "mapsme";
}

void OsrmRouter::ClearState()
{
  m_requestCancel = true;

  threads::MutexGuard guard(m_routeMutex);
  UNUSED_VALUE(guard);

  m_cachedFinalNodes.clear();

  m_dataFacade.Clear();
  m_mapping.Clear();

  m_container.Close();
}

void OsrmRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  {
    threads::MutexGuard guard(m_paramsMutex);
    UNUSED_VALUE(guard);

    m_finalPt = finalPt;
    m_isFinalChanged = true;

    m_requestCancel = true;
  }
}

void OsrmRouter::CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback, m2::PointD const & direction)
{
  {
    threads::MutexGuard guard(m_paramsMutex);
    UNUSED_VALUE(guard);

    m_startPt = startPt;
    m_startDr = direction;

    m_requestCancel = true;
  }

  GetPlatform().RunAsync(bind(&OsrmRouter::CalculateRouteAsync, this, callback));
}

void OsrmRouter::CalculateRouteAsync(ReadyCallback const & callback)
{
  if (m_isReadyThread.test_and_set())
    return;

  Route route(GetName());
  ResultCode code;

  threads::MutexGuard guard(m_routeMutex);
  UNUSED_VALUE(guard);

  m_isReadyThread.clear();

  m2::PointD startPt, finalPt, startDr;
  {
    threads::MutexGuard params(m_paramsMutex);
    UNUSED_VALUE(params);

    startPt = m_startPt;
    finalPt = m_finalPt;
    startDr = m_startDr;

    if (m_isFinalChanged)
      m_cachedFinalNodes.clear();
    m_isFinalChanged = false;

    m_requestCancel = false;
  }

  try
  {
    code = CalculateRouteImpl(startPt, startDr, finalPt, route);
    switch (code)
    {
    case StartPointNotFound:
      LOG(LWARNING, ("Can't find start point node"));
      break;
    case EndPointNotFound:
      LOG(LWARNING, ("Can't find end point node"));
      break;
    case PointsInDifferentMWM:
      LOG(LWARNING, ("Points are in different MWMs"));
      break;
    case RouteNotFound:
      LOG(LWARNING, ("Route not found"));
      break;
    case RouteFileNotExist:
      LOG(LWARNING, ("There are no routing file"));
      break;

    default:
      break;
    }
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Routing index absent or incorrect. Error while loading routing index:", e.Msg()));
    code = InternalError;

    // Clear data while m_container is valid.
    m_dataFacade.Clear();
    m_mapping.Clear();

    m_container.Close();
  }

  GetPlatform().RunOnGuiThread(bind(callback, route, code));
}

namespace
{

bool IsRouteExist(RawRouteData const & r)
{
  return !(INVALID_EDGE_WEIGHT == r.shortest_path_length ||
          r.segment_end_coordinates.empty() ||
          r.source_traversed_in_reverse.empty());
}

}

OsrmRouter::ResultCode OsrmRouter::CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt, Route & route)
{
  // 1. Find country file name and check that we are in the same MWM.
  string fName = m_countryFn(startPt);
  if (fName != m_countryFn(finalPt))
    return PointsInDifferentMWM;

  // 2. Open routing file index if needed.
  Platform & pl = GetPlatform();
  fName += DATA_FILE_EXTENSION;

  string const fPath = pl.WritablePathForFile(fName + ROUTING_FILE_EXTENSION);
  if (!pl.IsFileExistsByFullPath(fPath))
    return RouteFileNotExist;

  if (NeedReload(fPath))
  {
    LOG(LDEBUG, ("Load routing index for file:", fPath));

    // Clear data while m_container is valid.
    m_dataFacade.Clear();
    m_mapping.Clear();

    // Open new container and check that mwm and routing have equal timestamp.
    m_container.Open(fPath);
    {
      FileReader r1 = m_container.GetReader(VERSION_FILE_TAG);
      ReaderSrc src1(r1);
      ModelReaderPtr r2 = FilesContainerR(pl.GetReader(fName)).GetReader(VERSION_FILE_TAG);
      ReaderSrc src2(r2.GetPtr());

      if (ver::ReadTimestamp(src1) != ver::ReadTimestamp(src2))
      {
        m_container.Close();
        return InconsistentMWMandRoute;
      }
    }

    m_mapping.Load(m_container);
  }

  // 3. Find start/end nodes.
  if (!m_mapping.IsMapped())
    m_mapping.Map(m_container);

  FeatureGraphNodeVecT graphNodes;
  uint32_t mwmId = -1;
  ResultCode const code = FindPhantomNodes(fName, startPt, startDr, finalPt, graphNodes, MAX_NODE_CANDIDATES, mwmId);

  if (m_requestCancel)
    return Cancelled;

  m_mapping.Unmap();

  if (code != NoError)
    return code;

  // 4. Find route.
  m_dataFacade.Load(m_container);

  ASSERT_EQUAL(graphNodes.size(), MAX_NODE_CANDIDATES * 2, ());

  SearchEngineData engineData;
  ShortestPathRouting<DataFacadeT> pathFinder(&m_dataFacade, engineData);
  RawRouteData rawRoute;

  size_t ni = 0, nj = 0;
  while (ni < MAX_NODE_CANDIDATES && nj < MAX_NODE_CANDIDATES)
  {
    if (m_requestCancel)
      break;

    PhantomNodes nodes;
    nodes.source_phantom = graphNodes[ni].m_node;
    nodes.target_phantom = graphNodes[nj + MAX_NODE_CANDIDATES].m_node;

    rawRoute = RawRouteData();

    if ((nodes.source_phantom.forward_node_id != INVALID_NODE_ID ||
         nodes.source_phantom.reverse_node_id != INVALID_NODE_ID) &&
        (nodes.target_phantom.forward_node_id != INVALID_NODE_ID ||
         nodes.target_phantom.reverse_node_id != INVALID_NODE_ID))
    {
      rawRoute.segment_end_coordinates.push_back(nodes);

      pathFinder({nodes}, {}, rawRoute);
    }

    if (!IsRouteExist(rawRoute))
    {
      ++ni;
      if (ni == MAX_NODE_CANDIDATES)
      {
        ++nj;
        ni = 0;
      }
    }
    else
      break;
  }

  //m_dataFacade.Clear();

  if (!IsRouteExist(rawRoute))
    return RouteNotFound;

  if (m_requestCancel)
    return Cancelled;

  // 5. Restore route.
  m_mapping.Map(m_container);

  ASSERT_LESS(ni, MAX_NODE_CANDIDATES, ());
  ASSERT_LESS(nj, MAX_NODE_CANDIDATES, ());

  typedef OsrmFtSegMapping::FtSeg SegT;

  FeatureGraphNode const & sNode = graphNodes[ni];
  FeatureGraphNode const & eNode = graphNodes[nj + MAX_NODE_CANDIDATES];

  SegT const & segBegin = sNode.m_seg;
  SegT const & segEnd = eNode.m_seg;

  ASSERT(segBegin.IsValid(), ());
  ASSERT(segEnd.IsValid(), ());

  Route::TurnsT turnsDir;
  uint32_t estimateTime = 0;

  vector<m2::PointD> points;
  for (auto i : osrm::irange<std::size_t>(0, rawRoute.unpacked_path_segments.size()))
  {
    if (m_requestCancel)
      return Cancelled;

    // Get all the coordinates for the computed route
    size_t const n = rawRoute.unpacked_path_segments[i].size();
    for (size_t j = 0; j < n; ++j)
    {
      PathData const & path_data = rawRoute.unpacked_path_segments[i][j];

      // turns
      if (j > 0)
      {
        estimateTime += path_data.segment_duration;

        Route::TurnItem t;
        t.m_index = points.size() - 1;

        GetTurnDirection(rawRoute.unpacked_path_segments[i][j - 1],
                         rawRoute.unpacked_path_segments[i][j],
                         mwmId, t);
        if (t.m_turn != turns::NoTurn)
          turnsDir.push_back(t);
      }

      buffer_vector<SegT, 8> buffer;
      m_mapping.ForEachFtSeg(path_data.node, MakeBackInsertFunctor(buffer));

      auto correctFn = [&buffer] (SegT const & seg, size_t & ind)
      {
        auto const it = find_if(buffer.begin(), buffer.end(), [&seg] (OsrmFtSegMapping::FtSeg const & s)
        {
          return s.IsIntersect(seg);
        });

        ASSERT(it != buffer.end(), ());
        ind = distance(buffer.begin(), it);
      };

      //m_mapping.DumpSegmentByNode(path_data.node);

      size_t startK = 0, endK = buffer.size();
      if (j == 0)
        correctFn(segBegin, startK);
      if (j == n - 1)
      {
        correctFn(segEnd, endK);
        ++endK;
      }

      for (size_t k = startK; k < endK; ++k)
      {
        SegT const & seg = buffer[k];

        FeatureType ft;
        Index::FeaturesLoaderGuard loader(*m_pIndex, mwmId);
        loader.GetFeature(seg.m_fid, ft);
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        auto startIdx = seg.m_pointStart;
        auto endIdx = seg.m_pointEnd;

        if (j == 0 && k == startK)
          startIdx = (seg.m_pointEnd > seg.m_pointStart) ? segBegin.m_pointStart : segBegin.m_pointEnd;
        if (j == n - 1 && k == endK - 1)
          endIdx = (seg.m_pointEnd > seg.m_pointStart) ? segEnd.m_pointEnd : segEnd.m_pointStart;

        if (seg.m_pointEnd > seg.m_pointStart)
        {
          for (auto idx = startIdx; idx <= endIdx; ++idx)
            points.push_back(ft.GetPoint(idx));
        }
        else
        {
          for (auto idx = startIdx; idx > endIdx; --idx)
            points.push_back(ft.GetPoint(idx));
          points.push_back(ft.GetPoint(endIdx));
        }
      }

    }
  }

  points.front() = sNode.m_segPt;
  points.back() = eNode.m_segPt;

  if (points.size() < 2)
    return RouteNotFound;

  turnsDir.push_back(Route::TurnItem(points.size() - 1, turns::ReachedYourDestination));
  FixupTurns(points, turnsDir);

  // osrm multiple seconds to 10, so we need to divide it back
  estimateTime /= 10;

#ifdef _DEBUG
  for (auto t : turnsDir)
  {
    LOG(LDEBUG, (turns::GetTurnString(t.m_turn), ":", t.m_index, t.m_srcName, "-", t.m_trgName, "exit:", t.m_exitNum));
  }
#endif

  route.SetGeometry(points.begin(), points.end());
  route.SetTurnInstructions(turnsDir);
  route.SetTime(estimateTime);

  LOG(LDEBUG, ("Estimate time:", estimateTime, "s"));

  return NoError;
}
m2::PointD OsrmRouter::GetPointForTurnAngle(OsrmFtSegMapping::FtSeg const &seg,
                                            FeatureType const &ft, m2::PointD const &turnPnt,
                                            size_t (*GetPndInd)(const size_t, const size_t, const size_t)) const
{
  const size_t maxPntsNum = 5;
  const double maxDistMeter = 250.f;
  double curDist = 0.f;
  m2::PointD pnt = turnPnt, nextPnt;

  const size_t segDist = abs(seg.m_pointEnd - seg.m_pointStart);
  ASSERT_LESS(segDist, ft.GetPointsCount(),
              ("GetPntForTurnAngle(). The start and the end pnt of a segment are too far from each other"));
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

NodeID OsrmRouter::GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData)
{
  ASSERT_NOT_EQUAL(src, SPECIAL_NODEID, ());
  ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());
  if (!edgeData.shortcut)
    return trg;

  ASSERT_LESS(edgeData.id, m_dataFacade.GetNumberOfNodes(), ());
  EdgeID edge = SPECIAL_EDGEID;
  QueryEdge::EdgeData d;
  for (EdgeID e : m_dataFacade.GetAdjacentEdgeRange(edgeData.id))
  {
    if (m_dataFacade.GetTarget(e) == src )
    {
      d = m_dataFacade.GetEdgeData(e, edgeData.id);
      if (d.backward)
      {
        edge = e;
        break;
      }
    }
  }

  if (edge == SPECIAL_EDGEID)
  {
    for (EdgeID e : m_dataFacade.GetAdjacentEdgeRange(src))
    {
      if (m_dataFacade.GetTarget(e) == edgeData.id)
      {
        d = m_dataFacade.GetEdgeData(e, src);
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
    return GetTurnTargetNode(src, edgeData.id, d);

  return edgeData.id;
}

void OsrmRouter::GetPossibleTurns(NodeID node,
                                  m2::PointD const & p1,
                                  m2::PointD const & p,
                                  uint32_t mwmId,
                                  OsrmRouter::TurnCandidatesT & candidates)
{

  for (EdgeID e : m_dataFacade.GetAdjacentEdgeRange(node))
  {
    QueryEdge::EdgeData const data = m_dataFacade.GetEdgeData(e, node);
    if (!data.forward)
      continue;

    NodeID trg = GetTurnTargetNode(node, m_dataFacade.GetTarget(e), data);
    ASSERT_NOT_EQUAL(trg, SPECIAL_NODEID, ());

    auto const range = m_mapping.GetSegmentsRange(trg);
    OsrmFtSegMapping::FtSeg seg;
    m_mapping.GetSegmentByIndex(range.first, seg);

    FeatureType ft;
    Index::FeaturesLoaderGuard loader(*m_pIndex, mwmId);
    loader.GetFeature(seg.m_fid, ft);
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    m2::PointD const p2 = ft.GetPoint(seg.m_pointStart < seg.m_pointEnd ? seg.m_pointStart + 1 : seg.m_pointStart - 1);
    ASSERT_EQUAL(p, ft.GetPoint(seg.m_pointStart), ());

    double a = my::RadToDeg(ang::AngleTo(p, p2) - ang::AngleTo(p, p1));
    while (a < 0)
      a += 360;

    candidates.emplace_back(a, trg);
  }

  sort(candidates.begin(), candidates.end(), [](TurnCandidate const & t1, TurnCandidate const & t2) { return t1.m_angle < t2.m_angle; });
  auto last = unique(candidates.begin(), candidates.end());
  candidates.erase(last, candidates.end());
}


void OsrmRouter::GetTurnDirection(PathData const & node1,
                                  PathData const & node2,
                                  uint32_t mwmId, Route::TurnItem & turn)
{

  auto nSegs1 = m_mapping.GetSegmentsRange(node1.node);
  auto nSegs2 = m_mapping.GetSegmentsRange(node2.node);

  ASSERT_GREATER(nSegs1.second, 0, ());

  OsrmFtSegMapping::FtSeg seg1, seg2;
  m_mapping.GetSegmentByIndex(nSegs1.second - 1, seg1);
  m_mapping.GetSegmentByIndex(nSegs2.first, seg2);

  FeatureType ft1, ft2;
  Index::FeaturesLoaderGuard loader1(*m_pIndex, mwmId);
  Index::FeaturesLoaderGuard loader2(*m_pIndex, mwmId);

  loader1.GetFeature(seg1.m_fid, ft1);
  loader2.GetFeature(seg2.m_fid, ft2);

  ft1.ParseGeometry(FeatureType::BEST_GEOMETRY);
  ft2.ParseGeometry(FeatureType::BEST_GEOMETRY);

  ASSERT_LESS(MercatorBounds::DistanceOnEarth(ft1.GetPoint(seg1.m_pointEnd), ft2.GetPoint(seg2.m_pointStart)), 2, ());

  m2::PointD p = ft1.GetPoint(seg1.m_pointEnd);
  m2::PointD p1 = GetPointForTurnAngle(seg1, ft1, p,
                    [](const size_t start, const size_t end, const size_t i)
                    {
                      return end > start ? end - i : end + i;
                    });
  m2::PointD p2 = GetPointForTurnAngle(seg2, ft2, p,
                    [](const size_t start, const size_t end, const size_t i)
                    {
                      return end > start ? start + i : start - i;
                    });

  double a = my::RadToDeg(ang::AngleTo(p, p2) - ang::AngleTo(p, p1));
  while (a < 0)
    a += 360;

#ifdef _DEBUG
  TurnCandidatesT nodes;
  GetPossibleTurns(node1.node, p1, p, mwmId, nodes);

  LOG(LDEBUG, ("Possible turns: ", nodes.size()));
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    TurnCandidate const &t = nodes[i];
    LOG(LDEBUG, ("Angle:", t.m_angle, "Node:", t.m_node));
  }
#endif

  turn.m_turn = turns::NoTurn;
  if (a >= 23 && a < 67)
    turn.m_turn = turns::TurnSharpRight;
  else if (a >= 67 && a < 130)
    turn.m_turn = turns::TurnRight;
  else if (a >= 130 && a < 165)
    turn.m_turn = turns::TurnSlightRight;
  else if (a >= 165 && a < 195)
    turn.m_turn = turns::GoStraight;
  else if (a >= 195 && a < 230)
    turn.m_turn = turns::TurnSlightLeft;
  else if (a >= 230 && a < 292)
    turn.m_turn = turns::TurnLeft;
  else if (a >= 292 && a < 336)
    turn.m_turn = turns::TurnSharpLeft;

  bool const isRound1 = ftypes::IsRoundAboutChecker::Instance()(ft1);
  bool const isRound2 = ftypes::IsRoundAboutChecker::Instance()(ft2);

  auto hasMultiTurns = [&]()
  {
    for (EdgeID e : m_dataFacade.GetAdjacentEdgeRange(node1.node))
    {
      QueryEdge::EdgeData const & d = m_dataFacade.GetEdgeData(e, node1.node);
      if (d.forward && m_dataFacade.GetTarget(e) != node2.node)
        return true;
    }
    return false;
  };

  if (isRound1 && isRound2)
  {
    if (hasMultiTurns())
      turn.m_turn = turns::StayOnRoundAbout;
    else // No turn possible.
      turn.m_turn = turns::NoTurn;
    return;
  }

  if (!isRound1 && isRound2)
  {
    turn.m_turn = turns::EnterRoundAbout;
    return;
  }

  if (isRound1 && !isRound2)
  {
    STATIC_ASSERT(turns::TurnRight < turns::TurnSlightRight);
    STATIC_ASSERT(turns::TurnLeft < turns::TurnSlightLeft);

    /// @todo: add support of left-hand traffic
    if (!turns::IsLeftTurn(turn.m_turn))
      turn.m_turn = turns::LeaveRoundAbout;
    return;
  }

  // get names
  string name1, name2;
  {
    ft1.GetName(FeatureType::DEFAULT_LANG, turn.m_srcName);
    ft2.GetName(FeatureType::DEFAULT_LANG, turn.m_trgName);

    search::GetStreetNameAsKey(turn.m_srcName, name1);
    search::GetStreetNameAsKey(turn.m_trgName, name2);
  }

  string road1 = ft1.GetRoadNumber();
  string road2 = ft2.GetRoadNumber();

  if ((!name1.empty() && name1 == name2) ||
      (!road1.empty() && road1 == road2))
  {
    turn.m_turn = turns::NoTurn;
    return;
  }

  if (turn.m_turn == turns::GoStraight)
  {
    if (!hasMultiTurns())
      turn.m_turn = turns::NoTurn;

    return;
  }

  if (turn.m_turn == turns::NoTurn)
    turn.m_turn = turns::UTurn;
}

void OsrmRouter::FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir) const
{
  uint32_t exitNum = 0;
  Route::TurnItem * roundabout = 0;

  auto distance = [&points](uint32_t start, uint32_t end)
  {
    double res = 0.0;
    for (uint32_t i = start + 1; i < end; ++i)
      res += MercatorBounds::DistanceOnEarth(points[i - 1], points[i]);
    return res;
  };

  for (uint32_t idx = 0; idx < turnsDir.size(); )
  {
    Route::TurnItem & t = turnsDir[idx];
    if (roundabout && t.m_turn != turns::StayOnRoundAbout && t.m_turn != turns::LeaveRoundAbout)
    {
      exitNum = 0;
      roundabout = 0;
    }
    else if (t.m_turn == turns::EnterRoundAbout)
    {
      ASSERT_EQUAL(roundabout, 0, ());
      roundabout = &t;
    }
    else if (t.m_turn == turns::StayOnRoundAbout)
      ++exitNum;
    else if (t.m_turn == turns::LeaveRoundAbout)
    {
      roundabout->m_exitNum = exitNum + 1;
      roundabout = 0;
      exitNum = 0;
    }

    double const mergeDist = 30.0;

    if (idx > 0 &&
        turns::IsStayOnRoad(turnsDir[idx - 1].m_turn) &&
        turns::IsLeftOrRightTurn(turnsDir[idx].m_turn) &&
        distance(turnsDir[idx - 1].m_index, turnsDir[idx].m_index) < mergeDist)
    {
      turnsDir.erase(turnsDir.begin() + idx - 1);
      continue;
    }

    ++idx;
  }
}

IRouter::ResultCode OsrmRouter::FindPhantomNodes(string const & fName, m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt,
                                                 FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId)
{
  Point2PhantomNode getter(m_mapping, m_pIndex, startDr);

  auto processPt = [&] (m2::PointD const & p, size_t idx)
  {
    getter.SetPoint(p, idx);

    m_pIndex->ForEachInRectForMWM(getter,
          MercatorBounds::RectByCenterXYAndSizeInMeters(p, FEATURE_BY_POINT_RADIUS_M),
          scales::GetUpperScale(), fName);
  };

  processPt(startPt, 0);
  if (!getter.HasCandidates(0))
    return StartPointNotFound;

  bool const hasFinalCache = !m_cachedFinalNodes.empty();
  if (!hasFinalCache)
  {
    processPt(finalPt, 1);
    if (!getter.HasCandidates(1))
      return EndPointNotFound;
  }

  getter.MakeResult(res, maxCount, mwmId, !hasFinalCache, m_requestCancel);
  if (hasFinalCache)
    copy(m_cachedFinalNodes.begin(), m_cachedFinalNodes.end(), res.begin() + maxCount);
  else
    m_cachedFinalNodes.assign(res.begin() + maxCount, res.end());

  return NoError;
}

bool OsrmRouter::NeedReload(string const & fPath) const
{
  return (m_container.GetName() != fPath);
}

}
