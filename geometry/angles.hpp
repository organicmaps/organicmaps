#pragma once

#include "point2d.hpp"

#include "../base/start_mem_debug.hpp"

namespace ang
{
  /// Returns an angle of vector [p1, p2] from x-axis directed to y-axis.
  /// Angle is in range [-pi, pi].
  template <typename T>
  inline T AngleTo(m2::Point<T> const & p1, m2::Point<T> const & p2)
  {
    return atan2(p2.y - p1.y, p2.x - p1.x);
  }

  inline double GetMiddleAngle(double a1, double a2)
  {
    double ang = (a1 + a2) / 2.0;

    if (fabs(a1 - a2) > math::pi)
    {
      if (ang > 0.0)
        ang -= math::pi;
      else
        ang += math::pi;

    }
    return ang;
  }

  /// Average angle calcker. Can't find any suitable solution, so decided to do like this:
  /// Avg(i) = Avg(Avg(i-1), Ai);
  class AverageCalc
  {
    double m_ang;
    bool m_isEmpty;

  public:
    AverageCalc() : m_ang(0.0), m_isEmpty(true) {}

    void Add(double a)
    {
      m_ang = (m_isEmpty ? a : GetMiddleAngle(m_ang, a));
      m_isEmpty = false;
    }

    double GetAverage() const
    {
      return m_ang;
    }
  };
}

#include "../base/stop_mem_debug.hpp"
