#pragma once

#include "routing_common/num_mwm_id.hpp"

#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"
#include "geometry/tree4d.hpp"

#include "base/assert.hpp"

#include <unordered_set>

namespace routing
{
// Each MWM may have two canonical bounds. Query overlapping world copies and emit each ID once.
template <typename Fn>
void ForEachNumMwmInRect(m4::Tree<NumMwmId> const & tree, m2::RectD rect, Fn && fn)
{
  ASSERT(rect.IsValid(), (rect));
  rect = mercator::WrapRectX(rect);
  // Keep the inflated query whole so strict tree intersections include antimeridian contacts.
  rect.Inflate(kMwmPointAccuracy, kMwmPointAccuracy);
  std::unordered_set<NumMwmId> visited;
  auto const visit = [&](NumMwmId id)
  {
    if (visited.insert(id).second)
      fn(id);
  };
  tree.ForEachInRect(rect, visit);
  if (rect.minX() < mercator::Bounds::kMinX)
    rect.Offset(mercator::Bounds::kRangeX, 0.0);
  else if (rect.maxX() > mercator::Bounds::kMaxX)
    rect.Offset(-mercator::Bounds::kRangeX, 0.0);
  else
    return;
  tree.ForEachInRect(rect, visit);
}
}  // namespace routing
