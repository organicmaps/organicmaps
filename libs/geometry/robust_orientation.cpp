#include "geometry/robust_orientation.hpp"

#include "base/macros.hpp"

#include <algorithm>

extern "C"
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#include "3party/robust/predicates.c"
#pragma clang diagnostic pop
#else
#include "3party/robust/predicates.c"
#endif
}

namespace m2::robust
{
bool Init()
{
  exactinit();
  return true;
}

double OrientedS(PointD const & p1, PointD const & p2, PointD const & p)
{
  static bool const res = Init();
  ASSERT_EQUAL(res, true, ());
  UNUSED_VALUE(res);

  double a[] = {p1.x, p1.y};
  double b[] = {p2.x, p2.y};
  double c[] = {p.x, p.y};

  return orient2d(a, b, c);
}

bool IsSegmentInCone(PointD const & v, PointD const & v1, PointD const & vPrev, PointD const & vNext)
{
  double const cpLR = OrientedS(vPrev, vNext, v);

  if (cpLR == 0.0)
  {
    // Points vPrev, v, vNext placed on one line;
    // use property that polygon has CCW orientation.
    return OrientedS(vPrev, vNext, v1) > 0.0;
  }

  if (cpLR < 0.0)
  {
    // vertex is concave
    return OrientedS(v, vPrev, v1) < 0.0 && OrientedS(v, vNext, v1) > 0.0;
  }
  // vertex is convex
  return OrientedS(v, vPrev, v1) < 0.0 || OrientedS(v, vNext, v1) > 0.0;
}
}  // namespace m2::robust
