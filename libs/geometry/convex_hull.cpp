#include "geometry/convex_hull.hpp"

#include "geometry/robust_orientation.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

namespace m2
{
namespace
{
// Checks whether (p1 - p) x (p2 - p) > 0.
bool IsCCW(PointD const & p1, PointD const & p2, PointD const & p, double eps)
{
  return robust::OrientedS(p1, p2, p) > eps;
}

bool IsContinuedBy(std::vector<PointD> const & hull, PointD const & p, double eps)
{
  auto const n = hull.size();
  if (n < 2)
    return true;

  auto const & p1 = hull[n - 2];
  auto const & p2 = hull[n - 1];

  // Checks whether (p2 - p1) x (p - p2) > 0.
  return IsCCW(p, p1, p2, eps);
}

std::vector<PointD> BuildConvexHull(std::vector<PointD> points, double eps)
{
  base::SortUnique(points);

  ASSERT(points.empty() || points.begin() == min_element(points.begin(), points.end()), ());

  if (points.size() < 3)
    return points;

  auto const pivot = points[0];

  std::sort(points.begin() + 1, points.end(), [&pivot, &eps](PointD const & lhs, PointD const & rhs)
  {
    if (IsCCW(lhs, rhs, pivot, eps))
      return true;
    if (IsCCW(rhs, lhs, pivot, eps))
      return false;
    return lhs.SquaredLength(pivot) < rhs.SquaredLength(pivot);
  });

  std::vector<PointD> hull;

  for (auto const & p : points)
  {
    while (!IsContinuedBy(hull, p, eps))
      hull.pop_back();
    hull.push_back(p);
  }

  return hull;
}
}  // namespace

ConvexHull::ConvexHull(std::vector<PointD> const & points, double eps) : m_hull(BuildConvexHull(points, eps)) {}
}  // namespace m2
