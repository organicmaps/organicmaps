#pragma once

#include "routing/fake_edges_container.hpp"
#include "routing/segmented_route.hpp"

#include "base/assert.hpp"

#include <memory>

namespace routing
{
// Immutable state captured by a full route build and reused as the baseline for subsequent adjustments.
class RouteAdjustmentContext final
{
public:
  RouteAdjustmentContext(std::unique_ptr<SegmentedRoute> route, std::unique_ptr<FakeEdgesContainer> fakeEdges)
    : m_route(std::move(route))
    , m_fakeEdges(std::move(fakeEdges))
  {
    CHECK(m_route, ());
    CHECK(m_fakeEdges, ());
  }

  SegmentedRoute const & GetRoute() const { return *m_route; }

  FakeEdgesContainer const & GetFakeEdges() const { return *m_fakeEdges; }

private:
  std::unique_ptr<SegmentedRoute> const m_route;
  std::unique_ptr<FakeEdgesContainer> const m_fakeEdges;
};

using RouteAdjustmentContextPtr = std::shared_ptr<RouteAdjustmentContext const>;
}  // namespace routing
