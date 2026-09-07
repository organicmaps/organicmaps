#pragma once

#include "routing/edge_estimator.hpp"
#include "routing/fake_edges_container.hpp"
#include "routing/routing_options.hpp"
#include "routing/segmented_route.hpp"

#include "base/assert.hpp"

#include <memory>

namespace routing
{
// Immutable state captured by a full route build and reused as the baseline for subsequent adjustments.
class RouteAdjustmentContext final
{
public:
  RouteAdjustmentContext(std::unique_ptr<SegmentedRoute> route, std::unique_ptr<FakeEdgesContainer> fakeEdges,
                         EdgeEstimator::Strategy strategy = EdgeEstimator::Strategy::Normal,
                         RoutingOptions::RoadType routingOptions = 0)
    : m_route(std::move(route))
    , m_fakeEdges(std::move(fakeEdges))
    , m_strategy(strategy)
    , m_routingOptions(routingOptions)
  {
    CHECK(m_route, ());
    CHECK(m_fakeEdges, ());
  }

  SegmentedRoute const & GetRoute() const { return *m_route; }

  FakeEdgesContainer const & GetFakeEdges() const { return *m_fakeEdges; }

  EdgeEstimator::Strategy GetStrategy() const { return m_strategy; }
  RoutingOptions::RoadType GetRoutingOptions() const { return m_routingOptions; }

private:
  std::unique_ptr<SegmentedRoute> const m_route;
  std::unique_ptr<FakeEdgesContainer> const m_fakeEdges;
  EdgeEstimator::Strategy const m_strategy;
  RoutingOptions::RoadType const m_routingOptions;
};

using RouteAdjustmentContextPtr = std::shared_ptr<RouteAdjustmentContext const>;
}  // namespace routing
