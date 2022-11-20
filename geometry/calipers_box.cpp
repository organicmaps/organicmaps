#include "geometry/calipers_box.hpp"

#include "geometry/bounding_box.hpp"
#include "geometry/convex_hull.hpp"
#include "geometry/line2d.hpp"
#include "geometry/polygon.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace m2
{
namespace
{
static_assert(std::numeric_limits<double>::has_infinity, "");

double constexpr kInf = std::numeric_limits<double>::infinity();

// Checks whether (p1 - p) x (p2 - p) >= 0.
bool IsCCWNeg(PointD const & p1, PointD const & p2, PointD const & p, double eps)
{
  return robust::OrientedS(p1, p2, p) > -eps;
}

PointD Ort(PointD const & p) { return PointD(-p.y, p.x); }

// For each facet of the |hull| calls |fn| with the smallest rectangle
// containing the hull and with one side collinear to the facet.
template <typename Fn>
void ForEachRect(ConvexHull const & hull, Fn && fn)
{
  ASSERT_GREATER(hull.Size(), 2, ());

  size_t j = 0, k = 0, l = 0;
  for (size_t i = 0; i < hull.Size(); ++i)
  {
    auto const ab = hull.SegmentAt(i).Dir();

    j = std::max(j, i + 1);
    while (DotProduct(ab, hull.SegmentAt(j).Dir()) > CalipersBox::kEps)
      ++j;

    k = std::max(k, j);
    while (CrossProduct(ab, hull.SegmentAt(k).Dir()) > CalipersBox::kEps)
      ++k;

    l = std::max(l, k);
    while (DotProduct(ab, hull.SegmentAt(l).Dir()) < -CalipersBox::kEps)
      ++l;

    auto const oab = Ort(ab);
    std::array<Line2D, 4> const lines = {{Line2D(hull.PointAt(i), ab), Line2D(hull.PointAt(j), oab),
                                     Line2D(hull.PointAt(k), ab), Line2D(hull.PointAt(l), oab)}};
    std::vector<PointD> corners;
    for (size_t i = 0; i < lines.size(); ++i)
    {
      auto const j = (i + 1) % lines.size();
      auto result = Intersect(lines[i], lines[j], CalipersBox::kEps);
      if (result.m_type == IntersectionResult::Type::One)
        corners.push_back(result.m_point);
    }

    if (corners.size() != 4)
      continue;

    auto const it = min_element(corners.begin(), corners.end());
    rotate(corners.begin(), it, corners.end());

    fn(corners);
  }
}
}  // namespace

CalipersBox::CalipersBox(std::vector<PointD> const & points) : m_points({})
{
  ConvexHull hull(points, kEps);

  if (hull.Size() < 3)
  {
    m_points = hull.Points();
    return;
  }

  double bestArea = kInf;
  std::vector<PointD> bestPoints;
  ForEachRect(hull, [&](std::vector<PointD> const & points) {
    ASSERT_EQUAL(points.size(), 4, ());
    double const area = GetPolygonArea(points.begin(), points.end());
    if (area < bestArea)
    {
      bestArea = area;
      bestPoints = points;
    }
  });

  if (bestPoints.empty())
  {
    BoundingBox bbox(points);
    auto const min = bbox.Min();
    auto const max = bbox.Max();

    auto const width = max.x - min.x;
    auto const height = max.y - min.y;

    m_points.resize(4);
    m_points[0] = min;
    m_points[1] = m_points[0] + PointD(width, 0);
    m_points[2] = m_points[1] + PointD(0, height);
    m_points[3] = m_points[0] + PointD(0, height);
    return;
  }

  ASSERT_EQUAL(bestPoints.size(), 4, ());
  m_points = bestPoints;
}

bool CalipersBox::HasPoint(PointD const & p, double eps) const
{
  auto const n = m_points.size();

  if (n == 0)
    return false;

  if (n == 1)
    return AlmostEqualAbs(m_points[0], p, eps);

  if (n == 2)
    return IsPointOnSegmentEps(p, m_points[0], m_points[1], eps);

  for (size_t i = 0; i < n; ++i)
  {
    auto const & a = m_points[i];
    auto const & b = m_points[(i + 1) % n];
    if (!IsCCWNeg(b, p, a, eps))
      return false;
  }
  return true;
}
}  // namespace m2
