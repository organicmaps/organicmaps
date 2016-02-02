#include "triangle2d.hpp"

#include "robust_orientation.hpp"
#include "segment2d.hpp"

using namespace m2::robust;

namespace m2
{
bool IsPointInsideTriangle(m2::PointD const & pt, m2::PointD const & p1,
                           m2::PointD const & p2, m2::PointD const & p3)
{
  double const s1 = OrientedS(p1, p2, pt);
  double const s2 = OrientedS(p2, p3, pt);
  double const s3 = OrientedS(p3, p1, pt);

  // In the case of degenerate triangles we need to check that pt lies
  // on (p1, p2), (p2, p3) or (p3, p1).
  if (s1 == 0.0 && s2 == 0.0 && s3 == 0.0)
  {
    return IsPointInsideSegment(pt, p1, p2) || IsPointInsideSegment(pt, p2, p3) ||
           IsPointInsideSegment(pt, p3, p1);
  }

  return ((s1 >= 0.0 && s2 >= 0.0 && s3 >= 0.0) ||
          (s1 <= 0.0 && s2 <= 0.0 && s3 <= 0.0));
}

bool IsPointStrictlyInsideTriangle(m2::PointD const & pt, m2::PointD const & p1,
                                   m2::PointD const & p2, m2::PointD const & p3)
{
  double const s1 = OrientedS(p1, p2, pt);
  double const s2 = OrientedS(p2, p3, pt);
  double const s3 = OrientedS(p3, p1, pt);

  return ((s1 > 0.0 && s2 > 0.0 && s3 > 0.0) ||
          (s1 < 0.0 && s2 < 0.0 && s3 < 0.0));
}

} // namespace m2;
