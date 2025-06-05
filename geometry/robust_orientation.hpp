#pragma once

#include "geometry/point2d.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <iterator>

namespace m2
{
namespace robust
{
bool Init();

/// @return > 0, (p1, p2, p) - is CCW (left oriented)
///         < 0, (p1, p2, p) - is CW (right oriented)
/// Same as CrossProduct(p1 - p, p2 - p), but uses robust calculations.
double OrientedS(PointD const & p1, PointD const & p2, PointD const & p);

/// Is segment (v, v1) in cone (vPrev, v, vNext)?
/// @precondition (vPrev, v, vNext) is CCW.
bool IsSegmentInCone(PointD const & v, PointD const & v1, PointD const & vPrev, PointD const & vNext);

template <typename T>
bool Between(T a, T b, T c)
{
  return std::min(a, b) <= c && c <= std::max(a, b);
}

template <typename Point>
bool IsInSection(Point const & p1, Point const & p2, Point const & p)
{
  return Between(p1.x, p2.x, p.x) && Between(p1.y, p2.y, p.y);
}

template <typename Iter>
bool CheckPolygonSelfIntersections(Iter beg, Iter end)
{
  Iter last = end;
  --last;

  for (Iter i = beg; i != last; ++i)
  {
    for (Iter j = i; j != end; ++j)
    {
      // do not check intersection of neibour segments
      if (std::distance(i, j) <= 1 || (i == beg && j == last))
        continue;

      Iter ii = base::NextIterInCycle(i, beg, end);
      Iter jj = base::NextIterInCycle(j, beg, end);
      PointD a = *i, b = *ii, c = *j, d = *jj;

      // check for rect intersection
      if (std::max(a.x, b.x) < std::min(c.x, d.x) || std::min(a.x, b.x) > std::max(c.x, d.x) ||
          std::max(a.y, b.y) < std::min(c.y, d.y) || std::min(a.y, b.y) > std::max(c.y, d.y))
      {
        continue;
      }

      double const s1 = OrientedS(a, b, c);
      double const s2 = OrientedS(a, b, d);
      double const s3 = OrientedS(c, d, a);
      double const s4 = OrientedS(c, d, b);

      // check if sections have any intersection
      if (s1 * s2 > 0.0 || s3 * s4 > 0.0)
        continue;

      // Common principle if any point lay exactly on section, check 2 variants:
      // - only touching (><) - don't return as intersection;
      // - 'X'-crossing - return as intersection;
      // 'X'-crossing defines when points lay in different cones.

      if (s1 == 0.0 && IsInSection(a, b, c))
      {
        PointD const prev = *base::PrevIterInCycle(j, beg, end);

        PointD test[] = {a, b};
        if (a == c)
          test[0] = *base::PrevIterInCycle(i, beg, end);
        if (b == c)
          test[1] = *base::NextIterInCycle(ii, beg, end);

        if (IsSegmentInCone(c, test[0], prev, d) == IsSegmentInCone(c, test[1], prev, d))
          continue;
      }

      if (s2 == 0.0 && IsInSection(a, b, d))
      {
        PointD const next = *base::NextIterInCycle(jj, beg, end);

        PointD test[] = {a, b};
        if (a == d)
          test[0] = *base::PrevIterInCycle(i, beg, end);
        if (b == d)
          test[1] = *base::NextIterInCycle(ii, beg, end);

        if (IsSegmentInCone(d, test[0], c, next) == IsSegmentInCone(d, test[1], c, next))
          continue;
      }

      if (s3 == 0.0 && IsInSection(c, d, a))
      {
        PointD const prev = *base::PrevIterInCycle(i, beg, end);

        PointD test[] = {c, d};
        if (c == a)
          test[0] = *base::PrevIterInCycle(j, beg, end);
        if (d == a)
          test[1] = *base::NextIterInCycle(jj, beg, end);

        if (IsSegmentInCone(a, test[0], prev, b) == IsSegmentInCone(a, test[1], prev, b))
          continue;
      }

      if (s4 == 0.0 && IsInSection(c, d, b))
      {
        PointD const next = *base::NextIterInCycle(ii, beg, end);

        PointD test[] = {c, d};
        if (c == b)
          test[0] = *base::PrevIterInCycle(j, beg, end);
        if (d == b)
          test[1] = *base::NextIterInCycle(jj, beg, end);

        if (IsSegmentInCone(b, test[0], a, next) == IsSegmentInCone(b, test[1], a, next))
          continue;
      }

      return true;
    }
  }

  return false;
}
}  // namespace robust
}  // namespace m2
