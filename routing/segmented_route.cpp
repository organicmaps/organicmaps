#include "routing/segmented_route.hpp"

#include "routing/index_graph_starter.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>

namespace routing
{
SegmentedRoute::SegmentedRoute(m2::PointD const & start, m2::PointD const & finish)
  : m_start(start), m_finish(finish)
{
}

double SegmentedRoute::CalcDistance(m2::PointD const & point) const
{
  CHECK(!IsEmpty(), ());

  double result = std::numeric_limits<double>::max();
  for (auto const & step : m_steps)
    result = std::min(result, MercatorBounds::DistanceOnEarth(point, step.GetPoint()));

  return result;
}
}  // namespace routing
