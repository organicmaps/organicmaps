#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"
#include <cmath>

namespace m2
{
  /// axis aligned rect
  template <typename T>
  class AARect
  {
  private:

    Point<T> m_i;
    Point<T> m_j;
    Point<T> m_zero;
    Rect<T>  m_rect;

    Point<T> const Convert(Point<T> const & p,
                           Point<T> const & fromI,
                           Point<T> const & fromJ,
                           Point<T> const & toI,
                           Point<T> const & toJ)
    {
      Point<T> i(1, 0);
      Point<T> j(0, 1);

      Point<T> res;

      res.x = p.x * DotProduct(fromI, toI) + p.y * DotProduct(fromJ, toI);
      res.y = p.x * DotProduct(fromI, toJ) + p.y * DotProduct(fromJ, toJ);

      return res;
    }

    Point<T> const CoordConvertTo(Point<T> const & p)
    {
      Point<T> i(1, 0);
      Point<T> j(0, 1);

      Point<T> res;

      res.x = p.x * DotProduct(i, m_i) + p.y * DotProduct(j, m_i);
      res.y = p.x * DotProduct(i, m_j) + p.y * DotProduct(j, m_j);

      return res;
    }

    Point<T> const CoordConvertFrom(Point<T> const & p)
    {
      Point<T> res;

      Point<T> i(1, 0);
      Point<T> j(0, 1);

      res.x = p.x * DotProduct(m_i, i) + p.y * DotProduct(m_j, i);
      res.y = p.x * DotProduct(m_i, j) + p.y * DotProduct(m_j, j);

      return res;
    }

  public:

    AARect(Point<T> const & zero, T const & angle, Rect<T> const & r)
      : m_i(cos(angle), sin(angle)), m_j(-sin(angle), cos(angle)),
        m_zero(CoordConvertTo(zero)),
        m_rect(r)
    {
    }
    bool IsPointInside(Point<T> const & pt);

    bool IsIntersect(AARect<T> const & r);

    Point<T> const ConvertTo(Point<T> const & p)
    {
      m2::PointD i(1, 0);
      m2::PointD j(0, 1);
      return Convert(p - Convert(m_zero, m_i, m_j, i, j), i, j, m_i, m_j);
    }

    void ConvertTo(Point<T> * pts, size_t count)
    {
      for (size_t i = 0; i < count; ++i)
        pts[i] = ConvertTo(pts[i]);
    }

    Point<T> const ConvertFrom(Point<T> const & p)
    {
      m2::PointD i(1, 0);
      m2::PointD j(0, 1);
      return Convert(p + m_zero, m_i, m_j, i, j);
    }

    void ConvertFrom(Point<T> * pts, size_t count)
    {
      for (size_t i = 0; i < count; ++i)
        pts[i] = ConvertFrom(pts[i]);
    }

    Rect<T> const GetLocalRect()
    {
      return m_rect;
    }

    Rect<T> const GetGlobalRect()
    {
      Point<T> pts[4];
      GetGlobalPoints(pts);

      Rect<T> res(pts[0].x, pts[0].y, pts[0].x, pts[0].y);

      res.Add(pts[1]);
      res.Add(pts[2]);
      res.Add(pts[3]);

      return res;
    }

    void GetGlobalPoints(Point<T> * pts)
    {
      pts[0] = ConvertFrom(Point<T>(m_rect.minX(), m_rect.minY()));
      pts[1] = ConvertFrom(Point<T>(m_rect.minX(), m_rect.maxY()));
      pts[2] = ConvertFrom(Point<T>(m_rect.maxX(), m_rect.maxY()));
      pts[3] = ConvertFrom(Point<T>(m_rect.maxX(), m_rect.minY()));
    }
  };

  typedef AARect<double> AARectD;
  typedef AARect<float> AARectF;
}
