#include "osrm_router.hpp"
#include "osrm_data_facade.hpp"
#include "route.hpp"
#include "vehicle_model.hpp"

#include "../geometry/distance.hpp"

#include "../indexer/mercator.hpp"
#include "../indexer/index.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/SearchEngineData.h"
#include "../3party/osrm/osrm-backend/DataStructures/QueryEdge.h"
#include "../3party/osrm/osrm-backend/Descriptors/DescriptionFactory.h"
#include "../3party/osrm/osrm-backend/RoutingAlgorithms/ShortestPathRouting.h"


namespace routing
{

#define FACADE_READ_ZOOM_LEVEL  13

class Point2PhantomNode
{
  m2::PointD m_point;

  double m_minDist;
  uint32_t m_segIdx;
  FeatureID m_fID;

  OsrmFtSegMapping const & m_mapping;
  CarModel m_carModel;

public:
  Point2PhantomNode(m2::PointD const & pt, OsrmFtSegMapping const & mapping)
    : m_point(pt), m_minDist(numeric_limits<double>::max()), m_segIdx(-1), m_mapping(mapping)
  {
  }

  void operator() (FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE || m_carModel.GetSpeed(ft) == 0)
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const count = ft.GetPointsCount();
    ASSERT_GREATER(count, 1, ());
    for (size_t i = 1; i < count; ++i)
    {
      m2::DistanceToLineSquare<m2::PointD> segDist;
      segDist.SetBounds(ft.GetPoint(i - 1), ft.GetPoint(i));

      double const d = segDist(m_point);
      if (d < m_minDist)
      {
        m_minDist = d;
        m_fID = ft.GetID();
        m_segIdx = i - 1;
      }
    }
  }

  bool HasFeature() const
  {
    return m_fID.IsValid();
  }

  bool MakeResult(PhantomNode & resultNode, uint32_t & mwmId)
  {
    ASSERT(m_fID.IsValid(), ());

    resultNode.forward_node_id = resultNode.reverse_node_id = -1;

    OsrmFtSegMapping::FtSeg seg;
    seg.m_fid = m_fID.m_offset;
    seg.m_pointStart = m_segIdx;
    seg.m_pointEnd = m_segIdx + 1;

    m_mapping.GetOsrmNode(seg, resultNode.forward_node_id, resultNode.reverse_node_id);

    mwmId = m_fID.m_mwm;

    if (resultNode.forward_node_id == -1 && resultNode.reverse_node_id == -1)
    {
      LOG(LINFO, ("Can't find osrm node", seg));
      m_mapping.DumpSegmentsByFID(seg.m_fid);
      return false;
    }
    else
      return true;
  }

  size_t GetMwmID() const
  {
    return m_fID.m_mwm;
  }
};


// ----------------

OsrmRouter::OsrmRouter(Index const * index)
  : m_pIndex(index)
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
  typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;

#ifdef OMIM_OS_DESKTOP
  DataFacadeT facade("/Users/deniskoronchik/Documents/develop/omim-maps/Belarus.osrm");
  m_mapping.Load("/Users/deniskoronchik/Documents/develop/omim-maps/Belarus.osrm.ftseg");
#else
  DataFacadeT facade(GetPlatform().WritablePathForFile("Belarus.osrm"));
  m_mapping.Load(GetPlatform().WritablePathForFile("Belarus.osrm.ftseg"));
#endif
  SearchEngineData engine_working_data;
  ShortestPathRouting<DataFacadeT> shortest_path(&facade, engine_working_data);

  RawRouteData rawRoute;
  PhantomNodes nodes;
  MakeResultGuard resGuard(callback, GetName());

  uint32_t mwmIdStart = -1;
  if (!FindPhantomNode(startingPt, nodes.source_phantom, mwmIdStart))
  {
    resGuard.SetErrorMsg("Can't find start point");
    return;
  }

  uint32_t mwmIdEnd = -1;
  if (!FindPhantomNode(m_finalPt, nodes.target_phantom, mwmIdEnd))
  {
    resGuard.SetErrorMsg("Can't find end point");
    return;
  }

  if (mwmIdEnd != mwmIdStart || mwmIdEnd == -1 || mwmIdStart == -1)
  {
    resGuard.SetErrorMsg("Points in different MWMs");
    return;
  }

  FixedPointCoordinate startPoint((int)(MercatorBounds::YToLat(startingPt.y) * COORDINATE_PRECISION),
                                  (int)(MercatorBounds::XToLon(startingPt.x) * COORDINATE_PRECISION));

  FixedPointCoordinate endPoint((int)(MercatorBounds::YToLat(m_finalPt.y) * COORDINATE_PRECISION),
                                (int)(MercatorBounds::XToLon(m_finalPt.x) * COORDINATE_PRECISION));

  rawRoute.raw_via_node_coordinates = { startPoint, endPoint };
  rawRoute.segment_end_coordinates.push_back(nodes);

  shortest_path({nodes}, {}, rawRoute);

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
    for (PathData const & path_data : rawRoute.unpacked_path_segments[i])
    {
      auto const & v = m_mapping.GetSegVector(path_data.node);
      m_mapping.DumpSgementByNode(path_data.node);
      for (auto const & seg : v)
      {
        FeatureType ft;
        Index::FeaturesLoaderGuard loader(*m_pIndex, mwmIdStart);
        loader.GetFeature(seg.m_fid, ft);
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        if (seg.m_pointEnd > seg.m_pointStart)
        {
          for (auto j = seg.m_pointStart; j <= seg.m_pointEnd; ++j)
            points.push_back(ft.GetPoint(j));
        }
        else
        {
          for (auto j = seg.m_pointStart; j > seg.m_pointEnd; --j)
            points.push_back(ft.GetPoint(j));
          points.push_back(ft.GetPoint(seg.m_pointEnd));
        }
      }
    }
  }

  resGuard.SetGeometry(points);
}

bool OsrmRouter::FindPhantomNode(m2::PointD const & pt, PhantomNode & resultNode, uint32_t & mwmId)
{
  double radius = 100.0;

  Point2PhantomNode getter(pt, m_mapping);
  while (radius < 1000.0)
  {
    m_pIndex->ForEachInRect(getter,
                            MercatorBounds::RectByCenterXYAndSizeInMeters(pt, radius),
                            17);  /// @todo Pass minimal scale when ALL roads are visible.

    if (getter.HasFeature())
      return getter.MakeResult(resultNode, mwmId);
    else
      radius += 100;
  }

  return false;
}

}
