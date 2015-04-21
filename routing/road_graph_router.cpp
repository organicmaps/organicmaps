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
size_t const MAX_ROAD_CANDIDATES = 2;
double const FEATURE_BY_POINT_RADIUS_M = 100.0;

/// @todo Code duplication with road_graph.cpp
double TimeBetweenSec(m2::PointD const & v, m2::PointD const & w)
{
  static double const kMaxSpeedMPS = 5000.0 / 3600;
  return MercatorBounds::DistanceOnEarth(v, w) / kMaxSpeedMPS;
}

void AddFakeTurns(RoadPos const & rp, vector<RoadPos> const & vicinity,
                  vector<PossibleTurn> & turns)
{
  for (RoadPos const & vrp : vicinity)
  {
    PossibleTurn t;
    t.m_pos = vrp;
    /// @todo Do we need other fields? Do we even need m_secondsCovered?
    t.m_secondsCovered = TimeBetweenSec(rp.GetSegEndpoint(), vrp.GetSegEndpoint());
    turns.push_back(t);
  }
}
}  // namespace

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
  // Despite adding fake notes and calculating their vicinities on the fly
  // we still need to check that startPoint and finalPoint are in the same MWM
  // and probably reset the graph. So the checks stay here.
  vector<RoadPos> finalVicinity;
  size_t mwmID = GetRoadPos(finalPoint, finalVicinity);
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

  AddFakeTurns(startPos, startVicinity, m_roadGraph->m_startVicinityTurns);
  AddFakeTurns(finalPos, finalVicinity, m_roadGraph->m_finalVicinityTurns);

  vector<RoadPos> routePos;
  IRouter::ResultCode resultCode = CalculateRoute(startPos, finalPos, routePos);

  /// @todo They have fake feature ids. Can we still draw them?
  ASSERT(routePos.back() == finalPos, ());
  routePos.pop_back();
  ASSERT(routePos.front() == startPos, ());
  routePos.erase(routePos.begin());

  LOG(LINFO, ("Route calculation time:", timer.ElapsedSeconds(), "result code:", resultCode));

  m_roadGraph->ReconstructPath(routePos, route);
  return resultCode;
}

} // namespace routing
