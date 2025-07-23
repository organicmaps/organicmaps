#pragma once
#include "base/math.hpp"

#include "geometry/angles.hpp"
#include "geometry/rect2d.hpp"

namespace test
{
inline bool is_equal(double a1, double a2)
{
  return (fabs(a1 - a2) < 1.0E-10);
}

inline bool is_equal_atan(double x, double y, double v)
{
  return is_equal(ang::AngleTo(m2::PointD(0, 0), m2::PointD(x, y)), v);
}

inline bool is_equal_angle(double a1, double a2)
{
  double const two_pi = 2.0 * math::pi;
  if (a1 < 0.0)
    a1 += two_pi;
  if (a2 < 0.0)
    a2 += two_pi;

  return is_equal(a1, a2);
}

inline bool is_equal(m2::PointD const & p1, m2::PointD const & p2)
{
  return p1.EqualDxDy(p2, 1.0E-8);
}

inline bool is_equal_center(m2::RectD const & r1, m2::RectD const & r2)
{
  return is_equal(r1.Center(), r2.Center());
}

struct strict_equal
{
  bool operator()(m2::PointD const & p1, m2::PointD const & p2) const { return p1 == p2; }
};

struct epsilon_equal
{
  bool operator()(m2::PointD const & p1, m2::PointD const & p2) const { return is_equal(p1, p2); }
};
}  // namespace test
