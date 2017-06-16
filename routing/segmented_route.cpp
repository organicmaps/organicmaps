#include "routing/segmented_route.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <limits>

namespace routing
{
void SegmentedRoute::Init(m2::PointD const & start, m2::PointD const & finish)
{
  m_start = start;
  m_finish = finish;
  m_steps.clear();
}

double SegmentedRoute::CalcDistance(m2::PointD const & point) const
{
  CHECK(!IsEmpty(), ());

  double result = std::numeric_limits<double>::max();
  for (auto const & step : m_steps)
    result = std::min(result, MercatorBounds::DistanceOnEarth(point, step.GetPoint()));

  return result;
}

Segment const & SegmentedRoute::GetFinishSegment() const
{
  // Last segment is fake, before last is finish
  CHECK_GREATER(m_steps.size(), 2, ());
  return m_steps[m_steps.size() - 2].GetSegment();
}
}  // namespace routing
