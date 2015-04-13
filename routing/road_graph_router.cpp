#include "routing/features_road_graph.hpp"
#include "routing/nearest_road_pos_finder.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "geometry/distance.hpp"

#include "base/timer.hpp"
#include "base/logging.hpp"

namespace
{
size_t const MAX_ROAD_CANDIDATES = 2;
double const FEATURE_BY_POINT_RADIUS_M = 100.0;
}

namespace routing
{

RoadGraphRouter::~RoadGraphRouter()
{
}

RoadGraphRouter::RoadGraphRouter(Index const * pIndex, unique_ptr<IVehicleModel> && vehicleModel)
    : m_vehicleModel(move(vehicleModel)), m_pIndex(pIndex)
{
}

size_t RoadGraphRouter::GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos)
{
  NearestRoadPosFinder finder(pt, m2::PointD::Zero() /* undirected */, m_vehicleModel);
  auto f = [&finder](FeatureType & ft) { finder.AddInformationSource(ft); };
  m_pIndex->ForEachInRect(
      f, MercatorBounds::RectByCenterXYAndSizeInMeters(pt, FEATURE_BY_POINT_RADIUS_M),
      FeaturesRoadGraph::GetStreetReadScale());

  finder.MakeResult(pos, MAX_ROAD_CANDIDATES);
  return finder.GetMwmId();
}

bool RoadGraphRouter::IsMyMWM(size_t mwmID) const
{
  return m_roadGraph &&
         dynamic_cast<FeaturesRoadGraph const *>(m_roadGraph.get())->GetMwmID() == mwmID;
}

IRouter::ResultCode RoadGraphRouter::CalculateRoute(m2::PointD const & startPoint,
                                                    m2::PointD const & /* startDirection */,
                                                    m2::PointD const & finalPoint, Route & route)
{
  vector<RoadPos> finalPos;
  size_t mwmID = GetRoadPos(finalPoint, finalPos);
  if (!finalPos.empty() && !IsMyMWM(mwmID))
    m_roadGraph.reset(new FeaturesRoadGraph(m_pIndex, mwmID));

  if (!m_roadGraph)
    return EndPointNotFound;

  vector<RoadPos> startPos;
  mwmID = GetRoadPos(startPoint, startPos);

  if (startPos.empty() || !IsMyMWM(mwmID))
    return StartPointNotFound;

  my::Timer timer;
  timer.Reset();

  vector<RoadPos> routePos;

  IRouter::ResultCode resultCode = CalculateRouteM2M(startPos, finalPos, routePos);

  LOG(LINFO, ("Route calculation time:", timer.ElapsedSeconds(), "result code:", resultCode));

  m_roadGraph->ReconstructPath(routePos, route);
  return resultCode;
}

} // namespace routing
