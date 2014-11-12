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
  Point2PhantomNode(OsrmFtSegMapping const & mapping, Index const * pIndex)
    : m_mapping(mapping), m_ptIdx(0),
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

          node.m_node.forward_node_id = it->second.first;
          node.m_node.reverse_node_id = it->second.second;
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

void OsrmRouter::CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback)
{
  {
    threads::MutexGuard guard(m_paramsMutex);
    UNUSED_VALUE(guard);

    m_startPt = startPt;

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

  m2::PointD startPt, finalPt;
  {
    threads::MutexGuard params(m_paramsMutex);
    UNUSED_VALUE(params);

    startPt = m_startPt;
    finalPt = m_finalPt;

    if (m_isFinalChanged)
      m_cachedFinalNodes.clear();
    m_isFinalChanged = false;

    m_requestCancel = false;
  }

  try
  {
    code = CalculateRouteImpl(startPt, finalPt, route);
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

OsrmRouter::ResultCode OsrmRouter::CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & finalPt, Route & route)
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
  ResultCode const code = FindPhantomNodes(fName, startPt, finalPt, graphNodes, MAX_NODE_CANDIDATES, mwmId);

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

  m_dataFacade.Clear();

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

  Route::TurnsT turns;
  uint32_t estimateTime = 0;

  vector<m2::PointD> points;
  for (auto i : osrm::irange<std::size_t>(0, rawRoute.unpacked_path_segments.size()))
  {
    if (m_requestCancel)
      return Cancelled;

    // Get all the coordinates for the computed route
    m2::PointD p1, p;
    feature::TypesHolder fTypePrev;
    string namePrev;

    size_t const n = rawRoute.unpacked_path_segments[i].size();
    for (size_t j = 0; j < n; ++j)
    {
      PathData const & path_data = rawRoute.unpacked_path_segments[i][j];
      if (j != 0)
        estimateTime += path_data.segment_duration;

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


        if (j > 0 && k == startK)
        {
          ASSERT_LESS(MercatorBounds::DistanceOnEarth(p, ft.GetPoint(startIdx)), 2, ());

          m2::PointD const p2 = ft.GetPoint((seg.m_pointEnd > seg.m_pointStart) ? startIdx + 1 : startIdx - 1);

          string name;
          ft.GetName(FeatureType::DEFAULT_LANG, name);

          string n1, n2;
          search::GetStreetNameAsKey(namePrev, n1);
          search::GetStreetNameAsKey(name, n2);

          Route::TurnInstruction const t = GetTurnInstruction(fTypePrev, ft, p1, p, p2, (!n1.empty() && (n1 == n2)));

          if (t != Route::NoTurn)
            turns.push_back(Route::TurnItemT(points.size(), t));
        }

        if (k == endK - 1)
        {
          fTypePrev = feature::TypesHolder(ft);
          ft.GetName(FeatureType::DEFAULT_LANG, namePrev);
          p1 = ft.GetPoint((seg.m_pointEnd > seg.m_pointStart) ? (endIdx - 1) : (endIdx + 1));
          p = ft.GetPoint(endIdx);
        }

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

  // osrm multiple seconds to 10, so we need to divide it back
  estimateTime /= 10;

  route.SetGeometry(points.begin(), points.end());
  route.SetTurnInstructions(turns);
  route.SetTime(estimateTime);

  LOG(LDEBUG, ("Estimate time:", estimateTime, "s"));

  return NoError;
}

Route::TurnInstruction OsrmRouter::GetTurnInstruction(feature::TypesHolder const & ft1, feature::TypesHolder const & ft2,
                                                      m2::PointD const & p1, m2::PointD const & p, m2::PointD const & p2,
                                                      bool isStreetEqual) const
{
  bool const isRound1 = ftypes::IsRoundAboutChecker::Instance()(ft1);
  bool const isRound2 = ftypes::IsRoundAboutChecker::Instance()(ft2);

  if (isRound1 && isRound2)
    return Route::StayOnRoundAbout;

  if (!isRound1 && isRound2)
    return Route::EnterRoundAbout;

  if (isRound1 && !isRound2)
    return Route::LeaveRoundAbout;

  if (isStreetEqual)
    return Route::NoTurn;

  double a = my::RadToDeg(ang::AngleTo(p, p2) - ang::AngleTo(p, p1));
  while (a < 0)
    a += 360;

  if (a >= 23 && a < 67)
    return Route::TurnSharpRight;

  if (a >= 67 && a < 113)
    return Route::TurnRight;

  if (a >= 113 && a < 158)
    return Route::TurnSlightRight;

  if (a >= 158 && a < 202)
    return Route::GoStraight;

  if (a >= 202 && a < 248)
    return Route::TurnSlightLeft;

  if (a >= 248 && a < 292)
    return Route::TurnLeft;

  if (a >= 292 && a < 336)
    return Route::TurnSharpLeft;

  return Route::UTurn;
}

IRouter::ResultCode OsrmRouter::FindPhantomNodes(string const & fName, m2::PointD const & startPt, m2::PointD const & finalPt,
                                                 FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId)
{
  Point2PhantomNode getter(m_mapping, m_pIndex);

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
