#pragma once

#include "routing/directions_engine.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/router.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexGraph;

class SingleMwmRouter
{
public:
  SingleMwmRouter(string const & name, Index const & index, traffic::TrafficCache const & trafficCache,
                  shared_ptr<VehicleModelFactory> vehicleModelFactory,
                  shared_ptr<EdgeEstimator> estimator,
                  unique_ptr<IDirectionsEngine> directionsEngine);

  string const & GetName() const { return m_name; }

  IRouter::ResultCode CalculateRoute(MwmSet::MwmId const & mwmId, m2::PointD const & startPoint,
                                     m2::PointD const & startDirection,
                                     m2::PointD const & finalPoint, RouterDelegate const & delegate,
                                     Route & route);

  static unique_ptr<SingleMwmRouter> CreateCarRouter(Index const & index,
                                                     traffic::TrafficCache const & trafficCache);

private:
  IRouter::ResultCode DoCalculateRoute(MwmSet::MwmId const & mwmId, m2::PointD const & startPoint,
                                       m2::PointD const & startDirection,
                                       m2::PointD const & finalPoint,
                                       RouterDelegate const & delegate, Route & route);
  bool FindClosestEdge(MwmSet::MwmId const & mwmId, m2::PointD const & point,
                       Edge & closestEdge) const;
  bool LoadIndex(MwmSet::MwmId const & mwmId, string const & country, IndexGraph & graph);

  string const m_name;
  Index const & m_index;
  traffic::TrafficCache const & m_trafficCache;
  FeaturesRoadGraph m_roadGraph;
  shared_ptr<VehicleModelFactory> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;
  unique_ptr<IDirectionsEngine> m_directionsEngine;
};
}  // namespace routing
