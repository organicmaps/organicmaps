#include "osrm_router.hpp"
#include "route.hpp"
#include "vehicle_model.hpp"

#include "../geometry/distance.hpp"

#include "../indexer/mercator.hpp"
#include "../indexer/index.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/SearchEngineData.h"
#include "../3party/osrm/osrm-backend/Descriptors/DescriptionFactory.h"
#include "../3party/osrm/osrm-backend/RoutingAlgorithms/ShortestPathRouting.h"


namespace routing
{

#define FACADE_READ_ZOOM_LEVEL  13

class Point2PhantomNode
{
  m2::PointD m_point;
  OsrmFtSegMapping const & m_mapping;

  CarModel m_carModel;

  struct Candidate
  {
    double m_dist;
    uint32_t m_segIdx;
    FeatureID m_fID;
    m2::PointD m_point;

    Candidate() : m_dist(numeric_limits<double>::max()) {}
  };

  buffer_vector<Candidate, 128> m_candidates;

public:
  Point2PhantomNode(m2::PointD const & pt, OsrmFtSegMapping const & mapping)
    : m_point(pt), m_mapping(mapping)
  {
  }

  void operator() (FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE || m_carModel.GetSpeed(ft) == 0)
      return;

    Candidate res;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());
    for (size_t i = 1; i < count; ++i)
    {
      /// @todo Probably, we need to get exact proection distance in meters.
      m2::ProjectionToSection<m2::PointD> segProj;
      segProj.SetBounds(ft.GetPoint(i - 1), ft.GetPoint(i));

      m2::PointD const pt = segProj(m_point);
      double const d = m_point.SquareLength(pt);
      if (d < res.m_dist)
      {
        res.m_dist = d;
        res.m_fID = ft.GetID();
        res.m_segIdx = i - 1;
        res.m_point = pt;
      }
    }

    if (res.m_fID.IsValid())
      m_candidates.push_back(res);
  }

  bool MakeResult(PhantomNode & resultNode, uint32_t & mwmId, OsrmFtSegMapping::FtSeg & seg, m2::PointD & pt)
  {
    resultNode.forward_node_id = resultNode.reverse_node_id = -1;

    sort(m_candidates.begin(), m_candidates.end(), [] (Candidate const & r1, Candidate const & r2)
    {
      return (r1.m_dist < r2.m_dist);
    });

    for (auto const & c : m_candidates)
    {
      seg.m_fid = c.m_fID.m_offset;
      seg.m_pointStart = c.m_segIdx;
      seg.m_pointEnd = c.m_segIdx + 1;
      pt = c.m_point;
      m_mapping.GetOsrmNode(seg, resultNode.forward_node_id, resultNode.reverse_node_id);

      mwmId = c.m_fID.m_mwm;

      if (resultNode.forward_node_id != -1 || resultNode.reverse_node_id != -1)
        return true;
    }

    if (m_candidates.empty())
      LOG(LDEBUG, ("No candidates for point:", m_point));
    else
      LOG(LINFO, ("Can't find osrm node for feature:", m_candidates[0].m_fID));

    return false;
  }
};


// ----------------

OsrmRouter::OsrmRouter(Index const * index, CountryFileFnT const & fn)
  : m_countryFn(fn), m_pIndex(index)
{
}

string OsrmRouter::GetName() const
{
  return "mapsme";
}

void OsrmRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  m_finalPt = finalPt;
}

namespace
{

class MakeResultGuard
{
  IRouter::ReadyCallback const & m_callback;
  Route m_route;
  string m_errorMsg;

public:
  MakeResultGuard(IRouter::ReadyCallback const & callback, string const & name)
    : m_callback(callback), m_route(name)
  {
  }

  ~MakeResultGuard()
  {
    if (!m_errorMsg.empty())
      LOG(LINFO, ("Failed calculate route", m_errorMsg));
    m_callback(m_route);
  }

  void SetErrorMsg(char const * msg) { m_errorMsg = msg; }

  void SetGeometry(vector<m2::PointD> const & vec)
  {
    m_route.SetGeometry(vec.begin(), vec.end());
    m_errorMsg.clear();
  }
};

}

void OsrmRouter::CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback)
{
  MakeResultGuard resGuard(callback, GetName());

  string const fName = m_countryFn(startingPt);
  if (fName != m_countryFn(m_finalPt))
  {
    resGuard.SetErrorMsg("Points are in different MWMs");
    return;
  }

  if (m_lastMwmName != fName)
  {
    LOG(LDEBUG, ("Load routing index for file:", fName));
    try
    {
      // need to clear while m_fd is valid for handlers
      m_dataFacade.Clear();
      m_mapping.Clear();

      m_container.Open(GetPlatform().WritablePathForFile(fName + DATA_FILE_EXTENSION + ROUTING_FILE_EXTENSION));

      m_dataFacade.Load(m_container);
      m_mapping.Load(m_container);
    }
    catch(Reader::Exception const & e)
    {
      LOG(LERROR, ("Error while loading container", fName, e.Msg()));
      return;
    }

    m_lastMwmName = fName;
  }

  SearchEngineData engineData;
  ShortestPathRouting<DataFacadeT> pathFinder(&m_dataFacade, engineData);
  RawRouteData rawRoute;

  PhantomNodes nodes;

  OsrmFtSegMapping::FtSeg segBegin;
  m2::PointD segPointStart;
  uint32_t mwmIdStart = -1;
  if (!FindPhantomNode(startingPt, nodes.source_phantom, mwmIdStart, segBegin, segPointStart))
  {
    resGuard.SetErrorMsg("Can't find start point node");
    return;
  }

  uint32_t mwmIdEnd = -1;
  OsrmFtSegMapping::FtSeg segEnd;
  m2::PointD segPointEnd;
  if (!FindPhantomNode(m_finalPt, nodes.target_phantom, mwmIdEnd, segEnd, segPointEnd))
  {
    resGuard.SetErrorMsg("Can't find end point node");
    return;
  }

  if (mwmIdEnd != mwmIdStart || mwmIdEnd == -1 || mwmIdStart == -1)
  {
    resGuard.SetErrorMsg("Found features are in different MWMs");
    return;
  }

  rawRoute.segment_end_coordinates.push_back(nodes);

  pathFinder({nodes}, {}, rawRoute);

  if (INVALID_EDGE_WEIGHT == rawRoute.shortest_path_length
      || rawRoute.segment_end_coordinates.empty()
      || rawRoute.source_traversed_in_reverse.empty())
  {
    resGuard.SetErrorMsg("Route not found");
    return;
  }

  // restore route
  vector<m2::PointD> points;
  for (auto i : osrm::irange<std::size_t>(0, rawRoute.unpacked_path_segments.size()))
  {
    // Get all the coordinates for the computed route
    size_t n = rawRoute.unpacked_path_segments[i].size();
    for (size_t j = 0; j < n; ++j)
    {
      PathData const & path_data = rawRoute.unpacked_path_segments[i][j];
      pair<OsrmFtSegMapping::FtSeg const *, size_t> const range = m_mapping.GetSegVector(path_data.node);

      auto correctFn = [&range] (OsrmFtSegMapping::FtSeg const & seg, size_t & ind)
      {
        auto it = find_if(range.first, range.first + range.second, [&] (OsrmFtSegMapping::FtSeg const & s)
        {
          return s.IsIntersect(seg);
        });

        ASSERT(it != range.first + range.second, ());
        ind = distance(range.first, it);
      };

      //m_mapping.DumpSegmentByNode(path_data.node);

      size_t startK = 0, endK = range.second;
      if (j == 0)
        correctFn(segBegin, startK);
      else if (j == n - 1)
      {
        correctFn(segEnd, endK);
        ++endK;
      }

      for (size_t k = startK; k < endK; ++k)
      {
        auto const & seg = range.first[k];

        FeatureType ft;
        Index::FeaturesLoaderGuard loader(*m_pIndex, mwmIdStart);
        loader.GetFeature(seg.m_fid, ft);
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
        auto startIdx = seg.m_pointStart;
        auto endIdx = seg.m_pointEnd;

        if (j == 0 && k == startK)
          startIdx = (seg.m_pointEnd > seg.m_pointStart) ? segBegin.m_pointStart : segBegin.m_pointEnd;
        else if (j == n - 1 && k == endK - 1)
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

  points.front() = segPointStart;
  points.back() = segPointEnd;

  resGuard.SetGeometry(points);
}

bool OsrmRouter::FindPhantomNode(m2::PointD const & pt, PhantomNode & resultNode,
                                 uint32_t & mwmId, OsrmFtSegMapping::FtSeg & seg, m2::PointD & segPt)
{
  Point2PhantomNode getter(pt, m_mapping);

  /// @todo Review radius of rect and read index scale.
  m_pIndex->ForEachInRect(getter, MercatorBounds::RectByCenterXYAndSizeInMeters(pt, 1000.0), 17);

  return getter.MakeResult(resultNode, mwmId, seg, segPt);
}

}
