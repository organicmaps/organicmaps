#pragma once
#include "point2d.hpp"

#include "../base/math.hpp"

#include "../std/limits.hpp"
#include "../std/static_assert.hpp"

// Similarly to namespace m2 - 2d math, this is a namespace for nd math.
namespace mn
{

template <typename PointT> class DistanceToLineSquare
{
private:
  // we do not support unsigned points!!!
  STATIC_ASSERT(numeric_limits<typename PointT::value_type>::is_signed);

public:
  void SetBounds(PointT const & p0, PointT const & p1)
  {
    m_P0 = p0;
    m_P1 = p1;
    m_D = m_P1 - m_P0;
    m_D2 = DotProduct(m_D, m_D);

    m_D2 = sqrt(m_D2);
    if (my::AlmostEqual(m_D2, 0.0))
    {
      // make zero vector - then all DotProduct will be equal to zero
      m_D = m2::PointD(0, 0);
    }
    else
    {
      // normalize vector
      m_D = m_D / m_D2;
    }
  }

  double operator() (PointT Y) const
  {
    m2::PointD const YmP0 = Y - m_P0;
    double const t = DotProduct(m_D, YmP0);

    if (t <= 0)
    {
      // Y is closest to P0.
      return DotProduct(YmP0, YmP0);
    }
    if (t >= m_D2)
    {
      // Y is closest to P1.
      PointT const YmP1 = Y - m_P1;
      return DotProduct(YmP1, YmP1);
    }

    // Closest point is interior to segment.
    return my::sq(CrossProduct(YmP0, m_D));
  }

private:
  PointT m_P0, m_P1;
  m2::PointD m_D;
  double m_D2;
};

}
