#include "triangle2d.hpp"

#include "robust_orientation.hpp"


namespace m2
{

using namespace robust;

bool IsPointInsideTriangle(m2::PointD const & pt, m2::PointD const & p1,
                           m2::PointD const & p2, m2::PointD const & p3)
{
  double const s1 = OrientedS(p1, p2, pt);
  double const s2 = OrientedS(p2, p3, pt);
  double const s3 = OrientedS(p3, p1, pt);

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
