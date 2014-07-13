#include "road_graph_router.hpp"
#include "features_road_graph.hpp"
#include "route.hpp"

#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/index.hpp"

#include "../geometry/distance.hpp"


namespace routing
{

class Point2RoadPos
{
  m2::PointD m_point;
  double m_minDist;
  size_t m_segIdx;
  bool m_isOneway;
  FeatureID m_fID;

public:
  Point2RoadPos(m2::PointD const & pt)
    : m_point(pt), m_minDist(numeric_limits<double>::max())
  {
  }

  void operator() (FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE ||
        !ftypes::IsStreetChecker::Instance()(ft))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const count = ft.GetPointsCount() - 1;
    for (size_t i = 0; i < count; ++i)
    {
      m2::DistanceToLineSquare<m2::PointD> segDist;
      segDist.SetBounds(ft.GetPoint(i), ft.GetPoint(i + 1));
      double const d = segDist(m_point);
      if (d < m_minDist)
      {
        m_minDist = d;
        m_segIdx = i;
        m_fID = ft.GetID();
        m_isOneway = ftypes::IsStreetChecker::Instance().IsOneway(ft);
      }
    }
  }

  size_t GetMwmID() const
  {
    return m_fID.m_mwm;
  }

  void GetResults(vector<RoadPos> & res)
  {
    if (m_fID.IsValid())
    {
      res.push_back(RoadPos(m_fID.m_offset, true, m_segIdx));
      if (!m_isOneway)
        res.push_back(RoadPos(m_fID.m_offset, false, m_segIdx));
    }
  }
};

size_t RoadGraphRouter::GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos)
{
  Point2RoadPos getter(pt);
  m_pIndex->ForEachInRect(getter,
                          MercatorBounds::RectByCenterXYAndSizeInMeters(pt, 100.0),
                          FeatureRoadGraph::GetStreetReadScale());

  getter.GetResults(pos);
  return getter.GetMwmID();
}

bool RoadGraphRouter::IsMyMWM(size_t mwmID) const
{
  return (m_pRoadGraph && dynamic_cast<FeatureRoadGraph const *>(m_pRoadGraph.get())->GetMwmID() == mwmID);
}

void RoadGraphRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  vector<RoadPos> finalPos;
  size_t const mwmID = GetRoadPos(finalPt, finalPos);

  if (!finalPos.empty())
  {
    if (!IsMyMWM(mwmID))
      m_pRoadGraph.reset(new FeatureRoadGraph(m_pIndex, mwmID));

    SetFinalRoadPos(finalPos);
  }
}

void RoadGraphRouter::CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback)
{
  if (!m_pRoadGraph)
    return;

  vector<RoadPos> startPos;
  size_t const mwmID = GetRoadPos(startPt, startPos);

  if (startPos.empty() || !IsMyMWM(mwmID))
    return;

  vector<RoadPos> routePos;
  CalculateRoute(startPos, routePos);

  Route route;
  m_pRoadGraph->ReconstructPath(routePos, route);
  callback(route);
}

} // namespace routing
