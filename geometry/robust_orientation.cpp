#include "../base/SRC_FIRST.hpp"

#include "robust_orientation.hpp"

extern "C"
{
  #include "../3party/robust/predicates.c"
}

namespace m2 { namespace robust
{
  bool Init()
  {
    exactinit();
    return true;
  }

  double OrientedS(PointD const & p1, PointD const & p2, PointD const & p)
  {
    static bool res = Init();
    ASSERT_EQUAL ( res, true, () );

    double a[] = { p1.x, p1.y };
    double b[] = { p2.x, p2.y };
    double c[] = { p.x, p.y };

    return orient2d(a, b, c);
  }

  bool SegmentsIntersect(PointD const & a, PointD const & b,
                         PointD const & c, PointD const & d)
  {
    return
        max(a.x, b.x) >= min(c.x, d.x) &&
        min(a.x, b.x) <= max(c.x, d.x) &&
        max(a.y, b.y) >= min(c.y, d.y) &&
        min(a.y, b.y) <= max(c.y, d.y) &&
        OrientedS(a, b, c) * OrientedS(a, b, d) <= 0.0 &&
        OrientedS(c, d, a) * OrientedS(c, d, b) <= 0.0;
  }
}
}
