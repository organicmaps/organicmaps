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
}  // namespace

RoadGraphRouter::~RoadGraphRouter() {}

RoadGraphRouter::RoadGraphRouter(string const & name, Index & index,
                                 unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
                                 unique_ptr<IRoutingAlgorithm> && algorithm)
    : m_name(name)
    , m_algorithm(move(algorithm))
    , m_roadGraph(make_unique<FeaturesRoadGraph>(index, move(vehicleModelFactory)))
{
}

void RoadGraphRouter::ClearState()
{
  m_algorithm->Reset();
  m_roadGraph->ClearState();
}

IRouter::ResultCode RoadGraphRouter::CalculateRoute(m2::PointD const & startPoint,
                                                    m2::PointD const & /* startDirection */,
                                                    m2::PointD const & finalPoint, Route & route)
{
  vector<pair<Edge, m2::PointD>> finalVicinity;
  m_roadGraph->FindClosestEdges(finalPoint, MAX_ROAD_CANDIDATES, finalVicinity);
  
  if (finalVicinity.empty())
    return EndPointNotFound;

  vector<pair<Edge, m2::PointD>> startVicinity;
  m_roadGraph->FindClosestEdges(startPoint, MAX_ROAD_CANDIDATES, startVicinity);

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
                                                TRoutingVisualizerFn const & visualizerFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarRoutingAlgorithm(visualizerFn));
  unique_ptr<IRouter> router(new RoadGraphRouter("astar-pedestrian", index, move(vehicleModelFactory), move(algorithm)));
  return router;
}

unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(Index & index,
                                                             TRoutingProgressFn const & progressFn,
                                                             TRoutingVisualizerFn const & visualizerFn)
{
  unique_ptr<IVehicleModelFactory> vehicleModelFactory(new PedestrianModelFactory());
  unique_ptr<IRoutingAlgorithm> algorithm(new AStarBidirectionalRoutingAlgorithm(visualizerFn));
  unique_ptr<IRouter> router(new RoadGraphRouter("astar-bidirectional-pedestrian", index, move(vehicleModelFactory), move(algorithm)));
  return router;
}

}  // namespace routing
