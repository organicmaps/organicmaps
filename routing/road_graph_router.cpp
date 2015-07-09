#include "routing/features_road_graph.hpp"
#include "routing/nearest_edge_finder.hpp"
#include "routing/pedestrian_model.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/route.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "geometry/distance.hpp"

#include "base/assert.hpp"

namespace routing
{

namespace
{
// TODO (@gorshenin, @pimenov, @ldragunov): MAX_ROAD_CANDIDATES == 1
// means that only two closest feature will be examined when searching
// for features in the vicinity of start and final points.
// It is an oversimplification that is not as easily
// solved as tuning up this constant because if you set it too high
// you risk to find a feature that you cannot in fact reach because of
// an obstacle.  Using only the closest feature minimizes (but not
// eliminates) this risk.
size_t const MAX_ROAD_CANDIDATES = 1;

double const kMwmCrossingNodeEqualityRadiusMeters = 100.0;

IRouter::ResultCode Convert(IRoutingAlgorithm::Result value)
{
  switch (value)
  {
  case IRoutingAlgorithm::Result::OK: return IRouter::ResultCode::NoError;
  case IRoutingAlgorithm::Result::NoPath: return IRouter::ResultCode::RouteNotFound;
  case IRoutingAlgorithm::Result::Cancelled: return IRouter::ResultCode::Cancelled;
  }
  ASSERT(false, ("Unexpected IRoutingAlgorithm::Result value:", value));
  return IRouter::ResultCode::RouteNotFound;
}

string GetCountryForMwmFile(string const & mwmName)
{
  /// @todo Rework this function when storage will provide information about mwm's country
  // We assume following schemes:
  // Country or Country_Region
  size_t const pos = mwmName.find('_');
  if (string::npos == pos)
    return mwmName;
  return mwmName.substr(0, pos);
}
}  // namespace

RoadGraphRouter::~RoadGraphRouter() {}

RoadGraphRouter::RoadGraphRouter(string const & name, Index & index,
                                 unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
                                 unique_ptr<IRoutingAlgorithm> && algorithm,
                                 TMwmFileByPointFn const & countryFileFn)
    : m_name(name),
      m_index(index),
      m_vehicleModelFactory(move(vehicleModelFactory)),
      m_algorithm(move(algorithm)),
      m_countryFileFn(countryFileFn)
{
}

void RoadGraphRouter::GetClosestEdges(m2::PointD const & pt, vector<pair<Edge, m2::PointD>> & edges)
{
  NearestEdgeFinder finder(pt, *m_roadGraph.get());

  auto const f = [&finder, this](FeatureType & ft)
  {
    if (ft.GetFeatureType() == feature::GEOM_LINE && m_vehicleModel->GetSpeed(ft) > 0.0)
      finder.AddInformationSource(ft.GetID().m_offset);
  };

  m_index.ForEachInRect(
      f, MercatorBounds::RectByCenterXYAndSizeInMeters(pt, kMwmCrossingNodeEqualityRadiusMeters),
      FeaturesRoadGraph::GetStreetReadScale());

  finder.MakeResult(edges, MAX_ROAD_CANDIDATES);
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

  string const mwmName = m_countryFileFn(finalPoint);
  if (m_countryFileFn(startPoint) != mwmName)
    return PointsInDifferentMWM;

  platform::CountryFile countryFile(mwmName);
  MwmSet::MwmHandle const mwmHandle = m_index.GetMwmHandleByCountryFile(countryFile);
  if (!mwmHandle.IsAlive())
    return RouteFileNotExist;

  MwmSet::MwmId const mwmID = mwmHandle.GetId();
  if (!IsMyMWM(mwmID))
  {
    m_vehicleModel = m_vehicleModelFactory->GetVehicleModelForCountry(GetCountryForMwmFile(mwmName));
    m_roadGraph.reset(new FeaturesRoadGraph(*m_vehicleModel.get(), m_index, mwmID));
  }
  
  vector<pair<Edge, m2::PointD>> finalVicinity;
  GetClosestEdges(finalPoint, finalVicinity);
  
  if (finalVicinity.empty())
    return EndPointNotFound;

  vector<pair<Edge, m2::PointD>> startVicinity;
  GetClosestEdges(startPoint, startVicinity);

  if (startVicinity.empty())
    return StartPointNotFound;

  Junction const startPos(startPoint);
  Junction const finalPos(finalPoint);

  m_roadGraph->ResetFakes();
  m_roadGraph->AddFakeEdges(startPos, startVicinity);
  m_roadGraph->AddFakeEdges(finalPos, finalVicinity);

  vector<Junction> routePos;
  IRoutingAlgorithm::Result const resultCode = m_algorithm->CalculateRoute(*m_roadGraph, startPos, finalPos, routePos);

  m_roadGraph->ResetFakes();

  if (resultCode == IRoutingAlgorithm::Result::OK)
  {
    ASSERT_EQUAL(routePos.front(), startPos, ());
    ASSERT_EQUAL(routePos.back(), finalPos, ());
    m_roadGraph->ReconstructPath(routePos, route);
  }

  return Convert(resultCode);
}

unique_ptr<IRouter> CreatePedestrianAStarRouter(Index & index,
                                                TMwmFileByPointFn const & countryFileFn,
                                                TRoutingVisualizerFn const & visualizerFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarRoutingAlgorithm(visualizerFn));
  unique_ptr<IRouter> router(new RoadGraphRouter("astar-pedestrian", index, move(vehicleModelFactory), move(algorithm), countryFileFn));
  return router;
}

unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(
    Index & index, TMwmFileByPointFn const & countryFileFn,
    TRoutingVisualizerFn const & visualizerFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm(visualizerFn));
  unique_ptr<IRouter> router(new RoadGraphRouter("astar-bidirectional-pedestrian", index, move(vehicleModelFactory), move(algorithm), countryFileFn));
  return router;
}

}  // namespace routing
