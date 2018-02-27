#pragma once
#include "geometry/point2d.hpp"

#include "base/math.hpp"

#include <cmath>
#include <limits>

namespace m2
{

namespace impl
{

template <typename PointT> class CalculatedSection
{
private:
  static_assert(std::numeric_limits<typename PointT::value_type>::is_signed,
                "We do not support unsigned points!!!");

public:
  void SetBounds(PointT const & p0, PointT const & p1)
  {
    m_P0 = p0;
    m_P1 = p1;
    m_D = m_P1 - m_P0;
    m_D2 = Length(m_D);

    if (my::AlmostEqualULPs(m_D2, 0.0))
    {
      // make zero vector - then all DotProduct will be equal to zero
      m_D = m2::PointD::Zero();
    }
    else
    {
      // normalize vector
      m_D = m_D / m_D2;
    }
  }

  inline double GetLength() const { return m_D2; }
  inline PointT const & P0() const { return m_P0; }
  inline PointT const & P1() const { return m_P1; }

protected:
  template <class VectorT> static double SquareLength(VectorT const & v)
  {
    return DotProduct(v, v);
  }
  template <class VectorT> static double Length(VectorT const & v)
  {
    return sqrt(SquareLength(v));
  }
  double Distance(PointD const & v) const
  {
    return CrossProduct(v, m_D);
  }

  PointT m_P0, m_P1;
  m2::PointD m_D;
  double m_D2;
};

}  // namespace impl

template <typename PointT> class DistanceToLineSquare : public impl::CalculatedSection<PointT>
{
public:
  double operator() (PointT const & Y) const
  {
    m2::PointD const YmP0 = Y - this->m_P0;
    double const t = DotProduct(this->m_D, YmP0);

    if (t <= 0)
    {
      // Y is closest to P0.
      return this->SquareLength(YmP0);
    }
    if (t >= this->m_D2)
    {
      // Y is closest to P1.
      return this->SquareLength(Y - this->m_P1);
    }

    // Closest point is interior to segment.
    return std::pow(this->Distance(YmP0), 2);
  }
};

template <typename PointT> class ProjectionToSection : public impl::CalculatedSection<PointT>
{
public:
  m2::PointD operator() (PointT const & Y) const
  {
    m2::PointD const YmP0 = Y - this->m_P0;
    double const t = DotProduct(this->m_D, YmP0);

    if (t <= 0)
    {
      // Y is closest to P0.
      return this->m_P0;
    }
    if (t >= this->m_D2)
    {
      // Y is closest to P1.
      return this->m_P1;
    }

    return this->m_D*t + this->m_P0;
  }
};

}
