#pragma once

#include "routing/cross_mwm_index_graph.hpp"
#include "routing/directions_engine.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/joint.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/router.hpp"
#include "routing/routing_mapping.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace routing
{
class IndexGraph;
class IndexGraphStarter;

class IndexRouter : public IRouter
{
public:
  IndexRouter(string const & name, TCountryFileFn const & countryFileFn,
              shared_ptr<NumMwmIds> numMwmIds, shared_ptr<TrafficStash> trafficStash,
              shared_ptr<VehicleModelFactory> vehicleModelFactory,
              shared_ptr<EdgeEstimator> estimator, unique_ptr<IDirectionsEngine> directionsEngine,
              Index & index);

  // IRouter overrides:
  virtual string GetName() const override { return m_name; }
  virtual IRouter::ResultCode CalculateRoute(m2::PointD const & startPoint,
                                             m2::PointD const & startDirection,
                                             m2::PointD const & finalPoint,
                                             RouterDelegate const & delegate,
                                             Route & route) override;

  IRouter::ResultCode CalculateRouteForSingleMwm(string const & country,
                                                 m2::PointD const & startPoint,
                                                 m2::PointD const & startDirection,
                                                 m2::PointD const & finalPoint,
                                                 RouterDelegate const & delegate, Route & route);

  /// \note |numMwmIds| should not be null.
  static unique_ptr<IndexRouter> CreateCarRouter(TCountryFileFn const & countryFileFn,
                                                 shared_ptr<NumMwmIds> numMwmIds,
                                                 traffic::TrafficCache const & trafficCache,
                                                 Index & index);

private:
  IRouter::ResultCode CalculateRoute(string const & startCountry, string const & finishCountry,
                                     bool forSingleMwm, m2::PointD const & startPoint,
                                     m2::PointD const & startDirection,
                                     m2::PointD const & finalPoint, RouterDelegate const & delegate,
                                     Route & route);
  IRouter::ResultCode DoCalculateRoute(string const & startCountry, string const & finishCountry,
                                       bool forSingleMwm, m2::PointD const & startPoint,
                                       m2::PointD const & startDirection,
                                       m2::PointD const & finalPoint,
                                       RouterDelegate const & delegate, Route & route);
  bool FindClosestEdge(platform::CountryFile const & file, m2::PointD const & point,
                       Edge & closestEdge) const;
  // Input route may contains 'leaps': shortcut edges from mwm border enter to exit.
  // ProcessLeaps replaces each leap with calculated route through mwm.
  IRouter::ResultCode ProcessLeaps(vector<Segment> const & input, RouterDelegate const & delegate,
                                   IndexGraphStarter & starter, vector<Segment> & output);
  bool RedressRoute(vector<Segment> const & segments, RouterDelegate const & delegate,
                    bool forSingleMwm, IndexGraphStarter & starter, Route & route) const;

  string const m_name;
  Index & m_index;
  TCountryFileFn const m_countryFileFn;
  shared_ptr<NumMwmIds> m_numMwmIds;
  shared_ptr<TrafficStash> m_trafficStash;
  RoutingIndexManager m_indexManager;
  FeaturesRoadGraph m_roadGraph;
  shared_ptr<VehicleModelFactory> m_vehicleModelFactory;
  shared_ptr<EdgeEstimator> m_estimator;
  unique_ptr<IDirectionsEngine> m_directionsEngine;
};
}  // namespace routing
