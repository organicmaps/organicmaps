#include "routing/segmented_route.hpp"

#include "routing/index_graph_starter.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>

namespace routing
{
SegmentedRoute::SegmentedRoute(m2::PointD const & start, m2::PointD const & finish,
                               std::vector<Route::SubrouteAttrs> const & subroutes)
  : m_start(start)
  , m_finish(finish)
  , m_subroutes(subroutes)
{}

double SegmentedRoute::CalcDistance(m2::PointD const & point) const
{
  CHECK(!IsEmpty(), ());

  double result = std::numeric_limits<double>::max();
  for (auto const & step : m_steps)
    result = std::min(result, mercator::DistanceOnEarth(point, step.GetPoint()));

  return result;
}

Route::SubrouteAttrs const & SegmentedRoute::GetSubroute(size_t i) const
{
  CHECK_LESS(i, m_subroutes.size(), ());
  return m_subroutes[i];
}
}  // namespace routing
