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

// Checks whether (p1 - p) x (p2 - p) >= 0.
bool IsCCWNeg(PointD const & p1, PointD const & p2, PointD const & p, double eps)
{
  return robust::OrientedS(p1, p2, p) > -eps;
}

PointD Ort(PointD const & p)
{
  return PointD(-p.y, p.x);
}

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
    std::array<Line2D, 4> const lines = {Line2D(hull.PointAt(i), ab), Line2D(hull.PointAt(j), oab),
                                         Line2D(hull.PointAt(k), ab), Line2D(hull.PointAt(l), oab)};
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

    auto const it = std::min_element(corners.begin(), corners.end());
    std::rotate(corners.begin(), it, corners.end());

    fn(std::move(corners));
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

  double bestArea = std::numeric_limits<double>::max();
  std::vector<PointD> bestPoints;
  ForEachRect(hull, [&](std::vector<PointD> && points)
  {
    ASSERT_EQUAL(points.size(), 4, ());
    double const area = GetPolygonArea(points.begin(), points.end());
    if (area < bestArea)
    {
      bestArea = area;
      bestPoints = std::move(points);
    }
  });

  if (!bestPoints.empty())
  {
    ASSERT_EQUAL(bestPoints.size(), 4, ());
    m_points = std::move(bestPoints);
  }
  else
    m_points = BoundingBox(points).Points();
}

void CalipersBox::Deserialize(std::vector<PointD> && points)
{
  ASSERT_EQUAL(points.size(), 4, ());

  // 1. Stable after ser-des.
  m_points = std::move(points);
  ASSERT(TestValid(), ());

  // 2. Stable with input.
  // #ifdef DEBUG
  //   CalipersBox test(m_points);
  //   ASSERT(test.TestValid(), ());
  //   *this = std::move(test);
  // #endif
}

void CalipersBox::Normalize()
{
  m_points.erase(std::unique(m_points.begin(), m_points.end()), m_points.end());
  if (m_points.size() == 3)
  {
    if (m_points.front() == m_points.back())
      m_points.pop_back();
    else
      m_points.push_back(m_points.back());
  }
}

bool CalipersBox::TestValid() const
{
  size_t const n = m_points.size();
  for (size_t i = 0; i < n; ++i)
    if (!IsCCWNeg(m_points[i], m_points[(i + 1) % n], m_points[(i + 2) % n], kEps))
      return false;
  return n > 0;
}

bool CalipersBox::HasPoint(PointD const & p, double eps) const
{
  auto const n = m_points.size();
  switch (n)
  {
  case 0: return false;
  case 1: return AlmostEqualAbs(m_points[0], p, eps);
  case 2: return IsPointOnSegmentEps(p, m_points[0], m_points[1], eps);
  }

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
