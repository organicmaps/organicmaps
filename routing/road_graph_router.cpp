#include "routing/features_road_graph.hpp"
#include "routing/nearest_road_pos_finder.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "geometry/distance.hpp"

#include "base/assert.hpp"
#include "base/timer.hpp"
#include "base/logging.hpp"

namespace routing
{
namespace
{
// TODO (@gorshenin, @pimenov, @ldragunov): MAX_ROAD_CANDIDATES == 2
// means that only one closest feature (in both directions) will be
// examined when searching for features in the vicinity of start and
// final points. It is an oversimplification that is not as easily
// solved as tuning up this constant because if you set it too high
// you risk to find a feature that you cannot in fact reach because of
// an obstacle.  Using only the closest feature minimizes (but not
// eliminates) this risk.
size_t const MAX_ROAD_CANDIDATES = 2;
double const FEATURE_BY_POINT_RADIUS_M = 100.0;
}  // namespace

RoadGraphRouter::~RoadGraphRouter() {}

RoadGraphRouter::RoadGraphRouter(Index const * pIndex, unique_ptr<IVehicleModel> && vehicleModel)
    : m_vehicleModel(move(vehicleModel)), m_pIndex(pIndex)
{
}

MwmSet::MwmId RoadGraphRouter::GetRoadPos(m2::PointD const & pt, vector<RoadPos> & pos)
{
  NearestRoadPosFinder finder(pt, m2::PointD::Zero() /* undirected */, m_vehicleModel);
  auto f = [&finder](FeatureType & ft)
  {
    finder.AddInformationSource(ft);
  };
  m_pIndex->ForEachInRect(
      f, MercatorBounds::RectByCenterXYAndSizeInMeters(pt, FEATURE_BY_POINT_RADIUS_M),
      FeaturesRoadGraph::GetStreetReadScale());

  finder.MakeResult(pos, MAX_ROAD_CANDIDATES);
  return finder.GetMwmId();
}

bool RoadGraphRouter::IsMyMWM(MwmSet::MwmId const & mwmID) const
{
  return m_roadGraph &&
         dynamic_cast<FeaturesRoadGraph const *>(m_roadGraph.get())->GetMwmID() == mwmID;
}

IRouter::ResultCode RoadGraphRouter::CalculateRoute(m2::PointD const & startPoint,
                                                    m2::PointD const & /* startDirection */,
                                                    m2::PointD const & finalPoint, Route & route)
{
  // Despite adding fake notes and calculating their vicinities on the fly
  // we still need to check that startPoint and finalPoint are in the same MWM
  // and probably reset the graph. So the checks stay here.
  vector<RoadPos> finalVicinity;
  MwmSet::MwmId mwmID = GetRoadPos(finalPoint, finalVicinity);
  if (!finalVicinity.empty() && !IsMyMWM(mwmID))
    m_roadGraph.reset(new FeaturesRoadGraph(m_pIndex, mwmID));

  if (!m_roadGraph)
    return EndPointNotFound;

  vector<RoadPos> startVicinity;
  mwmID = GetRoadPos(startPoint, startVicinity);

  if (startVicinity.empty() || !IsMyMWM(mwmID))
    return StartPointNotFound;

  my::Timer timer;
  timer.Reset();

  RoadPos startPos(RoadPos::kFakeStartFeatureId, true /* forward */, 0 /* segId */, startPoint);
  RoadPos finalPos(RoadPos::kFakeFinalFeatureId, true /* forward */, 0 /* segId */, finalPoint);

  m_roadGraph->ResetFakeTurns();
  m_roadGraph->AddFakeTurns(startPos, startVicinity);
  m_roadGraph->AddFakeTurns(finalPos, finalVicinity);

  vector<RoadPos> routePos;
  IRouter::ResultCode const resultCode = CalculateRoute(startPos, finalPos, routePos);

  LOG(LINFO, ("Route calculation time:", timer.ElapsedSeconds(), "result code:", resultCode));

  if (IRouter::NoError == resultCode)
  {
    ASSERT(routePos.back() == finalPos, ());
    ASSERT(routePos.front() == startPos, ());

    m_roadGraph->ReconstructPath(routePos, route);
  }

  return resultCode;
}

}  // namespace routing
