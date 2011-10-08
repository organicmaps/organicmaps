#pragma once

#include <cmath>
#include "point2d.hpp"

#include "../base/matrix.hpp"
#include "../base/start_mem_debug.hpp"

namespace ang
{
  template <typename T>
  struct Angle
  {
  private:
    T m_val;
    T m_sin;
    T m_cos;
  public:
    Angle() : m_val(0), m_sin(0), m_cos(1)
    {}
    Angle(T const & val) : m_val(val), m_sin(::sin(val)), m_cos(::cos(val))
    {}
    Angle(T const & sin, T const & cos) : m_val(::atan2(sin, cos)), m_sin(sin), m_cos(cos)
    {}

    T const & val() const
    {
      return m_val;
    }

    T const & sin() const
    {
      return m_sin;
    }

    T const & cos() const
    {
      return m_cos;
    }

    Angle<T> const & operator*=(math::Matrix<T, 3, 3> const & m)
    {
      m2::Point<T> pt0(0, 0);
      m2::Point<T> pt1(m_cos, m_sin);

      pt1 *= m;
      pt0 *= m;

      m_val = atan2(pt1.y - pt0.y, pt1.x - pt0.x);
      m_sin = sin(m_val);
      m_cos = cos(m_val);

      return this;
    }
  };

  typedef Angle<double> AngleD;
  typedef Angle<float> AngleF;

  template <typename T>
  Angle<T> const operator*(Angle<T> const & a, math::Matrix<T, 3, 3> const & m)
  {
    m2::Point<T> pt0(0, 0);
    m2::Point<T> pt1(a.cos(), a.sin());

    pt1 *= m;
    pt0 *= m;

    return Angle<T>(atan2(pt1.y - pt0.y, pt1.x - pt0.x));
  }

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
