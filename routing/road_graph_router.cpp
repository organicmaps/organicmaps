#include "road_graph_router.hpp"
#include "features_road_graph.hpp"
#include "route.hpp"
#include "vehicle_model.hpp"

#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/index.hpp"

#include "../geometry/distance.hpp"

#include "../base/timer.hpp"

#include "../base/logging.hpp"


namespace routing
{

class Point2RoadPos
{
  m2::PointD m_point;
  double m_minDist;
  m2::PointD m_posBeg;
  m2::PointD m_posEnd;
  size_t m_segIdx;
  bool m_isOneway;
  FeatureID m_fID;
  IVehicleModel const * m_vehicleModel;

public:
  Point2RoadPos(m2::PointD const & pt, IVehicleModel const * vehicleModel)
    : m_point(pt), m_minDist(numeric_limits<double>::max()), m_vehicleModel(vehicleModel)
  {
  }

  void operator() (FeatureType const & ft)
  {
    if (ft.GetFeatureType() != feature::GEOM_LINE)
      return;

    double const speed = m_vehicleModel->GetSpeed(ft);
    if (speed <= 0.0)
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t const count = ft.GetPointsCount() - 1;
    for (size_t i = 0; i < count; ++i)
    {
      m2::DistanceToLineSquare<m2::PointD> segDist;
      m2::PointD const & p1 = ft.GetPoint(i);
      m2::PointD const & p2 = ft.GetPoint(i + 1);
      segDist.SetBounds(p1, p2);
      double const d = segDist(m_point);
      if (d < m_minDist)
      {
        m_minDist = d;
        m_segIdx = i;
        m_fID = ft.GetID();
        m_isOneway = m_vehicleModel->IsOneWay(ft);
        m_posBeg = p1;
        m_posEnd = p2;
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
      res.push_back(RoadPos(m_fID.m_offset, true, m_segIdx, m_posEnd));
      if (!m_isOneway)
        res.push_back(RoadPos(m_fID.m_offset, false, m_segIdx, m_posBeg));
    }
  }
};

RoadGraphRouter::~RoadGraphRouter()
{
}

RoadGraphRouter::RoadGraphRouter(Index const * pIndex) :
   m_vehicleModel(new CarModel()), m_pIndex(pIndex)
{

}

size_t RoadGraphRouter::GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos)
{
  Point2RoadPos getter(pt, m_vehicleModel.get());
  m_pIndex->ForEachInRect(getter,
                          MercatorBounds::RectByCenterXYAndSizeInMeters(pt, 100.0),
                          FeaturesRoadGraph::GetStreetReadScale());

  getter.GetResults(pos);
  return getter.GetMwmID();
}

bool RoadGraphRouter::IsMyMWM(size_t mwmID) const
{
  return (m_pRoadGraph && dynamic_cast<FeaturesRoadGraph const *>(m_pRoadGraph.get())->GetMwmID() == mwmID);
}

void RoadGraphRouter::SetFinalPoint(m2::PointD const & finalPt)
{
  vector<RoadPos> finalPos;
  size_t const mwmID = GetRoadPos(finalPt, finalPos);

  if (!finalPos.empty())
  {
    if (!IsMyMWM(mwmID))
      m_pRoadGraph.reset(new FeaturesRoadGraph(m_pIndex, mwmID));

    SetFinalRoadPos(finalPos);
  }
}

//void RoadGraphRouter::CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback)
void RoadGraphRouter::CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback, m2::PointD const & direction)
{
  if (!m_pRoadGraph)
    return;

  vector<RoadPos> startPos;
  size_t const mwmID = GetRoadPos(startingPt, startPos);

  if (startPos.empty() || !IsMyMWM(mwmID))
    return;

  my::Timer timer;
  timer.Reset();

  vector<RoadPos> routePos;
  CalculateRoute(startPos, routePos);

  LOG(LINFO, ("Route calculation time: ", timer.ElapsedSeconds()));

  Route route(GetName());
  m_pRoadGraph->ReconstructPath(routePos, route);
  callback(route, NoError);
}

} // namespace routing
