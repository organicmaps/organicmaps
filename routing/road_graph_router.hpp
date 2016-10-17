#pragma once

#include "routing/directions_engine.hpp"
#include "routing/road_graph.hpp"
#include "routing/router.hpp"
#include "routing/routing_algorithm.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Index;

namespace routing
{

class RoadGraphRouter : public IRouter
{
public:
  RoadGraphRouter(string const & name, Index const & index, TCountryFileFn const & countryFileFn,
                  IRoadGraph::Mode mode,
                  unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
                  unique_ptr<IRoutingAlgorithm> && algorithm,
                  unique_ptr<IDirectionsEngine> && directionsEngine);
  ~RoadGraphRouter() override;

  // IRouter overrides:
  string GetName() const override { return m_name; }
  void ClearState() override;
  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, RouterDelegate const & delegate,
                            Route & route) override;

private:
  void ReconstructRoute(vector<Junction> && junctions, Route & route,
                        my::Cancellable const & cancellable) const;

  /// Checks existance and add absent maps to route.
  /// Returns true if map exists
  bool CheckMapExistence(m2::PointD const & point, Route & route) const;

  string const m_name;
  TCountryFileFn const m_countryFileFn;
  Index const & m_index;
  unique_ptr<IRoutingAlgorithm> const m_algorithm;
  unique_ptr<IRoadGraph> const m_roadGraph;
  unique_ptr<IDirectionsEngine> const m_directionsEngine;
};

unique_ptr<IRouter> CreatePedestrianAStarRouter(Index & index, TCountryFileFn const & countryFileFn);
unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(Index & index, TCountryFileFn const & countryFileFn);
unique_ptr<IRouter> CreateBicycleAStarBidirectionalRouter(Index & index, TCountryFileFn const & countryFileFn);
unique_ptr<RoadGraphRouter> CreateCarAStarBidirectionalRouter(Index & index, TCountryFileFn const & countryFileFn);
}  // namespace routing
