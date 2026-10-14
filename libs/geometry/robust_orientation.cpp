#include "geometry/robust_orientation.hpp"

#if defined(__clang__)
#pragma float_control(push)
#pragma float_control(precise, on)
#elif defined(__GNUC__)
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

extern "C"
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconditional-uninitialized"
#include "3party/robust/predicates.c"
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include "3party/robust/predicates.c"
#pragma GCC diagnostic pop
#else
#include "3party/robust/predicates.c"
#endif
}

namespace m2::robust
{

struct DoInit
{
  DoInit() { exactinit(); }
} g_init;

double OrientedS(PointD const & p1, PointD const & p2, PointD const & p)
{
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
    // Vertex is convex: the cone is narrower than 180 degrees, so (v, v1) must be
    // on the inner side of both edges.
    return OrientedS(v, vPrev, v1) < 0.0 && OrientedS(v, vNext, v1) > 0.0;
  }
  // Vertex is concave: the cone is wider than 180 degrees, so one side is enough.
  return OrientedS(v, vPrev, v1) < 0.0 || OrientedS(v, vNext, v1) > 0.0;
}
}  // namespace m2::robust

#if defined(__clang__)
#pragma float_control(pop)
#elif defined(__GNUC__)
#pragma GCC pop_options
#elif defined(_MSC_VER)
#pragma fp_contract(on)
#endif
