#pragma once

#include "routing/base/astar_algorithm.hpp"
#include "routing/road_graph.hpp"
#include "routing/road_graph_router.hpp"
#include "std/function.hpp"

namespace routing
{
class AStarRouter : public RoadGraphRouter
{
public:
  AStarRouter(CountryFileFnT const & fn, Index const * pIndex = nullptr,
              RoutingVisualizerFn routingVisualizer = nullptr);

  // IRouter overrides:
  string GetName() const override { return "astar-pedestrian"; }
  void ClearState() override { Reset(); }

  // RoadGraphRouter overrides:
  ResultCode CalculateRoute(RoadPos const & startPos, RoadPos const & finalPos,
                            vector<RoadPos> & route) override;

  // my::Cancellable overrides:
  void Reset() override { m_algo.Reset(); }
  void Cancel() override { m_algo.Cancel(); }
  bool IsCancelled() const override { return m_algo.IsCancelled(); }

private:
  using TAlgorithm = AStarAlgorithm<RoadGraph>;
  TAlgorithm m_algo;
  RoutingVisualizerFn m_routingVisualizer;
};
}  // namespace routing
