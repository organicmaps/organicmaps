#include "geometry/cbox.hpp"

#include "geometry/bbox.hpp"
#include "geometry/convex_hull.hpp"
#include "geometry/line2d.hpp"
#include "geometry/polygon.hpp"
#include "geometry/segment2d.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <array>
#include <limits>

using namespace std;

namespace m2
{
namespace
{
static_assert(numeric_limits<double>::has_infinity, "");

double const kInf = numeric_limits<double>::infinity();
double const kEps = 1e-12;

// Checks whether (p1 - p) x (p2 - p) >= 0.
bool IsCCW(PointD const & p1, PointD const & p2, PointD const & p)
{
  return robust::OrientedS(p1, p2, p) > -kEps;
}

PointD Ort(PointD const & p) { return PointD(-p.y, p.x); }

template <typename Fn>
void ForEachRect(ConvexHull const & hull, Fn && fn)
{
  ASSERT_GREATER(hull.Size(), 2, ());

  size_t j = 0, k = 0, l = 0;
  for (size_t i = 0; i < hull.Size(); ++i)
  {
    auto const ab = hull.SegmentAt(i).Dir();

    j = max(j, i + 1);
    while (DotProduct(ab, hull.SegmentAt(j).Dir()) > kEps)
      ++j;

    k = max(k, j);
    while (CrossProduct(ab, hull.SegmentAt(k).Dir()) > kEps)
      ++k;

    l = max(l, k);
    while (DotProduct(ab, hull.SegmentAt(l).Dir()) < -kEps)
      ++l;

    auto const oab = Ort(ab);
    array<Line2D, 4> const lines = {{Line2D(hull.PointAt(i), ab), Line2D(hull.PointAt(j), oab),
                                     Line2D(hull.PointAt(k), ab), Line2D(hull.PointAt(l), oab)}};
    vector<m2::PointD> corners;
    for (size_t u = 0; u < lines.size(); ++u)
    {
      for (size_t v = u + 1; v < lines.size(); ++v)
      {
        auto result = LineIntersector::Intersect(lines[u], lines[v], kEps);
        if (result.m_type == LineIntersector::Result::Type::Single)
          corners.push_back(result.m_point);
      }
    }

    ConvexHull rect(corners);
    if (rect.Size() == 4)
      fn(rect.Points());
  }
}
}  // namespace

CBox::CBox(vector<m2::PointD> const & points) : m_points({})
{
  ConvexHull hull(points);

  if (hull.Size() <= 4)
  {
    m_points = hull.Points();
    return;
  }

  double bestArea = kInf;
  vector<m2::PointD> bestPoints;
  ForEachRect(hull, [&](vector<m2::PointD> const & points) {
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
    BBox bbox(points);
    auto const min = bbox.Min();
    auto const max = bbox.Max();

    auto const width = max.x - min.x;
    auto const height = max.y - min.y;

    m_points.resize(4);
    m_points[0] = min;
    m_points[1] = m_points[0] + m2::PointD(width, 0);
    m_points[2] = m_points[1] + m2::PointD(0, height);
    m_points[3] = m_points[0] + m2::PointD(0, height);
    return;
  }

  ASSERT_EQUAL(bestPoints.size(), 4, ());
  m_points = bestPoints;
}

bool CBox::IsInside(m2::PointD const & p) const
{
  auto const n = m_points.size();

  if (n == 0)
    return false;

  if (n == 1)
    return AlmostEqualAbs(m_points[0], p, kEps);

  if (n == 2)
    return IsPointOnSegmentEps(p, m_points[0], m_points[1], kEps);

  for (size_t i = 0; i < n; ++i) {
    auto const & a = m_points[i];
    auto const & b = m_points[(i + 1) % n];
    if (!IsCCW(b, p, a))
      return false;
  }
  return true;
}
}  // namespace m2
