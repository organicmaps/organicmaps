#pragma once
#include "../base/base.hpp"

// Similarly to namespace m2 - 2d math, this is a namespace for nd math.
namespace mn
{

template <typename PointT> class DistanceToLineSquare
{
public:
  DistanceToLineSquare(PointT p0, PointT p1)
    : m_P0(p0), m_P1(p1), m_D(m_P1 - m_P0), m_D2(DotProduct(m_D, m_D)), m_InvD2(1.0 / m_D2)
  {
  }

  double operator () (PointT Y) const
  {
    PointT const YmP0 = Y - m_P0;
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
    return DotProduct(YmP0, YmP0) - t * t * m_InvD2;
  }
private:
  PointT m_P0, m_P1, m_D;
  double m_D2, m_InvD2;
};

}
