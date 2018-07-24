#pragma once

#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <cmath>
#include <limits>

namespace m2
{
namespace impl
{
template <typename Point>
class CalculatedSection
{
public:
  void SetBounds(Point const & p0, Point const & p1)
  {
    m_p0 = p0;
    m_p1 = p1;
    m_d = m_p1 - m_p0;
    m_length = std::sqrt(SquaredLength(m_d));

    if (my::AlmostEqualULPs(m_length, 0.0))
    {
      // Make zero vector and all DotProduct will be equal to zero.
      m_d = m2::PointD::Zero();
    }
    else
    {
      // Normalize vector.
      m_d = m_d / m_length;
    }
  }

  double GetLength() const { return m_length; }
  Point const & P0() const { return m_p0; }
  Point const & P1() const { return m_p1; }

protected:
  template <typename Vector>
  static double SquaredLength(Vector const & v)
  {
    return DotProduct(v, v);
  }

  double Distance(PointD const & v) const { return CrossProduct(v, m_d); }

  Point m_p0;
  Point m_p1;
  m2::PointD m_d;
  double m_length;

private:
  static_assert(std::numeric_limits<typename Point::value_type>::is_signed,
                "Unsigned points are not supported");
};
}  // namespace impl

template <typename Point>
class DistanceToLineSquare : public impl::CalculatedSection<Point>
{
public:
  double operator()(Point const & p) const
  {
    Point const diff = p - this->m_p0;
    m2::PointD const diffD(diff);
    double const t = DotProduct(this->m_d, diffD);

    if (t <= 0)
      return this->SquaredLength(diffD);

    if (t >= this->m_length)
      return this->SquaredLength(p - this->m_p1);

    // Closest point is between |m_p0| and |m_p1|.
    return std::pow(this->Distance(diffD), 2.0);
  }
};

template <typename Point>
class ProjectionToSection : public impl::CalculatedSection<Point>
{
public:
  m2::PointD operator()(Point const & p) const
  {
    m2::PointD const diffD = p - this->m_p0;
    double const t = DotProduct(this->m_d, diffD);

    if (t <= 0)
      return this->m_p0;

    if (t >= this->m_length)
      return this->m_p1;

    return this->m_p0 + this->m_d * t;
  }
};
}  // namespace m2
