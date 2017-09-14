#include "geometry/convex_hull.hpp"

#include "geometry/robust_orientation.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

using namespace std;

namespace m2
{
namespace
{
double const kEps = 1e-9;

// Checks whether (p1 - p) x (p2 - p) > 0.
bool IsCCW(PointD const & p1, PointD const & p2, PointD const & p)
{
  return robust::OrientedS(p1, p2, p) > kEps;
}

bool ContinuesHull(vector<PointD> const & hull, PointD const & p)
{
  auto const n = hull.size();
  if (n < 2)
    return true;

  auto const & p1 = hull[n - 2];
  auto const & p2 = hull[n - 1];

  // Checks whether (p2 - p1) x (p - p2) > 0.
  return robust::OrientedS(p1, p, p2) < -kEps;
}
}  // namespace

vector<PointD> BuildConvexHull(vector<PointD> points)
{
  my::SortUnique(points);

  auto const n = points.size();

  if (n < 2)
    return points;

  iter_swap(points.begin(), min_element(points.begin(), points.end()));

  auto const pivot = points[0];

  sort(points.begin() + 1, points.end(), [&pivot](PointD const & lhs, PointD const & rhs) {
    if (IsCCW(lhs, rhs, pivot))
      return true;
    if (IsCCW(rhs, lhs, pivot))
      return false;
    return lhs.SquareLength(pivot) < rhs.SquareLength(pivot);
  });

  vector<PointD> hull;

  for (auto const & p : points)
  {
    while (!ContinuesHull(hull, p))
      hull.pop_back();
    hull.push_back(p);
  }

  return hull;
}
}  // namespace m2
