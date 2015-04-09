#pragma once

#include "geometry/point2d.hpp"

#include "base/stl_add.hpp"


namespace m2 { namespace robust
{
  bool Init();

  double OrientedS(PointD const & p1, PointD const & p2, PointD const & p);

  bool SegmentsIntersect(PointD const & p1, PointD const & p2,
                         PointD const & p3, PointD const & p4);

  template <typename T> bool Between(T a, T b, T c)
  {
    return min(a, b) <= c && c <= max(a, b);
  }

  template <class PointT> bool IsInSection(PointT const & p1, PointT const & p2, PointT const & p)
  {
    return Between(p1.x, p2.x, p.x) && Between(p1.y, p2.y, p.y);
  }

  template <typename IterT>
  bool CheckPolygonSelfIntersections(IterT beg, IterT end)
  {
    IterT last = end;
    --last;

    for (IterT i = beg; i != last; ++i)
      for (IterT j = i; j != end; ++j)
      {
        // do not check intersection of neibour segments
        if (distance(i, j) <= 1 || (i == beg && j == last))
          continue;

        IterT ii = NextIterInCycle(i, beg, end);
        IterT jj = NextIterInCycle(j, beg, end);
        PointD a = *i, b = *ii, c = *j, d = *jj;

        // check for rect intersection
        if (max(a.x, b.x) < min(c.x, d.x) ||
            min(a.x, b.x) > max(c.x, d.x) ||
            max(a.y, b.y) < min(c.y, d.y) ||
            min(a.y, b.y) > max(c.y, d.y))
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
        // - касание (><) - don't return as intersection;
        // - 'X'-crossing - return as intersection;
        // 'X'-crossing defines when points lay in different cones.

        if (s1 == 0.0 && IsInSection(a, b, c))
        {
          PointD const prev = *PrevIterInCycle(j, beg, end);

          PointD test[] = { a, b };
          if (a == c) test[0] = *PrevIterInCycle(i, beg, end);
          if (b == c) test[1] = *NextIterInCycle(ii, beg, end);

          if (IsSegmentInCone(c, test[0], prev, d) == IsSegmentInCone(c, test[1], prev, d))
            continue;
        }

        if (s2 == 0.0 && IsInSection(a, b, d))
        {
          PointD const next = *NextIterInCycle(jj, beg, end);

          PointD test[] = { a, b };
          if (a == d) test[0] = *PrevIterInCycle(i, beg, end);
          if (b == d) test[1] = *NextIterInCycle(ii, beg, end);

          if (IsSegmentInCone(d, test[0], c, next) == IsSegmentInCone(d, test[1], c, next))
            continue;
        }

        if (s3 == 0.0 && IsInSection(c, d, a))
        {
          PointD const prev = *PrevIterInCycle(i, beg, end);

          PointD test[] = { c, d };
          if (c == a) test[0] = *PrevIterInCycle(j, beg, end);
          if (d == a) test[1] = *NextIterInCycle(jj, beg, end);

          if (IsSegmentInCone(a, test[0], prev, b) == IsSegmentInCone(a, test[1], prev, b))
            continue;
        }

        if (s4 == 0.0 && IsInSection(c, d, b))
        {
          PointD const next = *NextIterInCycle(ii, beg, end);

          PointD test[] = { c, d };
          if (c == b) test[0] = *PrevIterInCycle(j, beg, end);
          if (d == b) test[1] = *NextIterInCycle(jj, beg, end);

          if (IsSegmentInCone(b, test[0], a, next) == IsSegmentInCone(b, test[1], a, next))
            continue;
        }

        return true;
      }

    return false;
  }
}
}
