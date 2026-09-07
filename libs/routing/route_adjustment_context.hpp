#pragma once

#include <memory>

namespace routing
{
// Router-specific immutable state used to adjust a previously calculated route.
class RouteAdjustmentContext
{
public:
  virtual ~RouteAdjustmentContext() = default;
};

using RouteAdjustmentContextPtr = std::shared_ptr<RouteAdjustmentContext const>;
}  // namespace routing
