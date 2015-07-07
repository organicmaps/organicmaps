#pragma once

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

typedef function<string(m2::PointD const &)> TMwmFileByPointFn;

class RoadGraphRouter : public IRouter
{
public:
  RoadGraphRouter(string const & name,
                  Index const & index,
                  unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
                  unique_ptr<IRoutingAlgorithm> && algorithm,
                  TMwmFileByPointFn const & countryFileFn);
  ~RoadGraphRouter() override;

  // IRouter overrides:
  string GetName() const override { return m_name; }
  void ClearState() override { Reset(); }
  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;

  // my::Cancellable overrides:
  void Reset() override { m_algorithm->Reset(); }
  void Cancel() override { m_algorithm->Cancel(); }
  bool IsCancelled() const override { return m_algorithm->IsCancelled(); }

private:
  /// @todo This method fits better in features_road_graph.
  void GetClosestEdges(m2::PointD const & pt, vector<pair<Edge, m2::PointD>> & edges);

  bool IsMyMWM(MwmSet::MwmId const & mwmID) const;

  string const m_name;
  Index const & m_index;
  unique_ptr<IVehicleModelFactory> const m_vehicleModelFactory;
  unique_ptr<IRoutingAlgorithm> const m_algorithm;
  TMwmFileByPointFn const m_countryFileFn;

  unique_ptr<IRoadGraph> m_roadGraph;
  shared_ptr<IVehicleModel> m_vehicleModel;
};
  
unique_ptr<IRouter> CreatePedestrianAStarRouter(Index const & index,
                                                TMwmFileByPointFn const & countryFileFn,
                                                TRoutingVisualizerFn const & visualizerFn);
unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(Index const & index,
                                                             TMwmFileByPointFn const & countryFileFn,
                                                             TRoutingVisualizerFn const & visualizerFn);

}  // namespace routing
