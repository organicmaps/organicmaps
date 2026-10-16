#pragma once

#include "routing_common/num_mwm_id.hpp"

#include "coding/point_coding.hpp"

#include "geometry/mercator.hpp"
#include "geometry/tree4d.hpp"

#include "base/assert.hpp"

#include <unordered_set>

namespace routing
{
// Stops when fn returns true for an overlapping MWM. Split bounds can test an ID more than once.
template <typename Fn>
bool ForAnyNumMwmInRect(m4::Tree<NumMwmId> const & tree, m2::RectD rect, Fn && fn)
{
  ASSERT(rect.IsValid(), (rect));
  rect = mercator::WrapRectX(rect);
  // Keep the inflated query whole so strict tree intersections include antimeridian contacts.
  rect.Inflate(kMwmPointAccuracy, kMwmPointAccuracy);
  if (tree.ForAnyInRect(rect, fn))
    return true;
  if (rect.minX() < mercator::Bounds::kMinX)
    rect.Offset(mercator::Bounds::kRangeX, 0.0);
  else if (rect.maxX() > mercator::Bounds::kMaxX)
    rect.Offset(-mercator::Bounds::kRangeX, 0.0);
  else
    return false;
  return tree.ForAnyInRect(rect, fn);
}

// Each MWM may have two canonical bounds. Emit each overlapping ID once.
template <typename Fn>
void ForEachNumMwmInRect(m4::Tree<NumMwmId> const & tree, m2::RectD const & rect, Fn && fn)
{
  std::unordered_set<NumMwmId> visited;
  ForAnyNumMwmInRect(tree, rect, [&](NumMwmId id)
  {
    if (visited.insert(id).second)
      fn(id);
    return false;
  });
}
}  // namespace routing
