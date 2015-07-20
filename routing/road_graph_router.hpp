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
  RoadGraphRouter(string const & name, Index & index,
                  unique_ptr<IVehicleModelFactory> && vehicleModelFactory,
                  unique_ptr<IRoutingAlgorithm> && algorithm);
  ~RoadGraphRouter() override;

  // IRouter overrides:
  string GetName() const override { return m_name; }
  void ClearState() override;
  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;

  // my::Cancellable overrides:
  void Reset() override { m_algorithm->Reset(); }
  void Cancel() override { m_algorithm->Cancel(); }
  bool IsCancelled() const override { return m_algorithm->IsCancelled(); }

private:
  string const m_name;
  unique_ptr<IRoutingAlgorithm> const m_algorithm;
  unique_ptr<IRoadGraph> const m_roadGraph;
};
  
unique_ptr<IRouter> CreatePedestrianAStarRouter(Index & index,
                                                TRoutingVisualizerFn const & visualizerFn);
unique_ptr<IRouter> CreatePedestrianAStarBidirectionalRouter(
    Index & index, TRoutingProgressFn const & progressFn,
    TRoutingVisualizerFn const & visualizerFn);

}  // namespace routing
