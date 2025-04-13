#include "search/geometry_utils.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

namespace search
{
double PointDistance(m2::PointD const & a, m2::PointD const & b)
{
  return mercator::DistanceOnEarth(a, b);
}

bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double eps)
{
  return m2::IsEqual(r1, r2, eps, eps);
}
}  // namespace search
