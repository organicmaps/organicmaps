#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"
#include "rect_intersect.hpp"
#include "angles.hpp"
#include "../base/math.hpp"
#include <cmath>

namespace m2
{
  /// axis aligned rect
  template <typename T>
  class AnyRect
  {
    ang::Angle<T> m_angle;

    /// @todo No need to store orthos separately. They are stored in m_angle.
    Point<T> m_i;
    Point<T> m_j;

    Point<T> m_zero;
    Rect<T>  m_rect;

    static Point<T> const Convert(Point<T> const & p,
                                  Point<T> const & fromI, Point<T> const & fromJ,
                                  Point<T> const & toI, Point<T> const & toJ)
    {
      Point<T> res;

      res.x = p.x * DotProduct(fromI, toI) + p.y * DotProduct(fromJ, toI);
      res.y = p.x * DotProduct(fromI, toJ) + p.y * DotProduct(fromJ, toJ);

      return res;
    }

  public:
    AnyRect() : m_i(1, 0), m_j(0, 1), m_zero(0, 0), m_rect() {}

    /// creating from regular rect
    explicit AnyRect(Rect<T> const & r)
      : m_angle(0), m_i(m_angle.cos(), m_angle.sin()), m_j(-m_angle.sin(), m_angle.cos())
    {
      if (r.IsValid())
      {
        m_zero = Point<T>(r.minX(), r.minY());
        m_rect = Rect<T>(0, 0, r.SizeX(), r.SizeY());
      }
      else
      {
        m_zero = Point<T>(0, 0);
        m_rect = r;
      }
    }

    AnyRect(Point<T> const & zero, ang::Angle<T> const & angle, Rect<T> const & r)
      : m_angle(angle), m_i(m_angle.cos(), m_angle.sin()), m_j(-m_angle.sin(), m_angle.cos()),
        m_zero(Convert(zero, Point<T>(1, 0), Point<T>(0, 1), m_i, m_j)),
        m_rect(r)
    {
    }

    Point<T> const & LocalZero() const
    {
      return m_zero;
    }

    Point<T> const GlobalZero() const
    {
      m2::Point<T> i(1, 0);
      m2::Point<T> j(0, 1);
      return Convert(m_zero, m_i, m_j, i, j);
    }

    Point<T> const & i() const
    {
      return m_i;
    }

    Point<T> const & j() const
    {
      return m_j;
    }

    void SetAngle(ang::Angle<T> const & a)
    {
      m2::Point<T> glbZero = GlobalZero();

      m_angle = a;
      m_i = m2::Point<T>(m_angle.cos(), m_angle.sin());
      m_j = m2::Point<T>(-m_angle.sin(), m_angle.cos());

      m_zero = Convert(glbZero, Point<T>(1, 0), Point<T>(0, 1), m_i, m_j);
    }

    ang::Angle<T> const & Angle() const
    {
      return m_angle;
    }

    Point<T> const GlobalCenter() const
    {
      return ConvertFrom(m_rect.Center());
    }

    Point<T> const LocalCenter() const
    {
      return m_rect.Center();
    }

    T GetMaxSize() const { return max(m_rect.SizeX(), m_rect.SizeY()); }

    bool IsPointInside(Point<T> const & pt) const
    {
      return m_rect.IsPointInside(ConvertTo(pt));
    }

    bool IsRectInside(AnyRect<T> const & r) const
    {
      m2::Point<T> pts[4];
      r.GetGlobalPoints(pts);
      ConvertTo(pts, 4);
      return m_rect.IsPointInside(pts[0])
          && m_rect.IsPointInside(pts[1])
          && m_rect.IsPointInside(pts[2])
          && m_rect.IsPointInside(pts[3]);
    }

    bool IsIntersect(AnyRect<T> const & r) const
    {
      m2::Point<T> pts[4];
      if (r.GetLocalRect() == Rect<T>())
        return false;
      r.GetGlobalPoints(pts);
      ConvertTo(pts, 4);

      m2::Rect<T> r1(pts[0], pts[0]);
      r1.Add(pts[1]);
      r1.Add(pts[2]);
      r1.Add(pts[3]);

      if (!GetLocalRect().IsIntersect(r1))
        return false;

      if (r.IsRectInside(*this))
        return true;

      if (IsRectInside(r))
        return true;

      return Intersect(GetLocalRect(), pts[0], pts[1])
          || Intersect(GetLocalRect(), pts[1], pts[2])
          || Intersect(GetLocalRect(), pts[2], pts[3])
          || Intersect(GetLocalRect(), pts[3], pts[0]);
    }

    /// Convert into coordinate system of this AnyRect
    Point<T> const ConvertTo(Point<T> const & p) const
    {
      m2::Point<T> i(1, 0);
      m2::Point<T> j(0, 1);
      return Convert(p - Convert(m_zero, m_i, m_j, i, j), i, j, m_i, m_j);
    }

    void ConvertTo(Point<T> * pts, size_t count) const
    {
      for (size_t i = 0; i < count; ++i)
        pts[i] = ConvertTo(pts[i]);
    }

    /// Convert into global coordinates from the local coordinates of this AnyRect
    Point<T> const ConvertFrom(Point<T> const & p) const
    {
      m2::Point<T> i(1, 0);
      m2::Point<T> j(0, 1);
      return Convert(p + m_zero, m_i, m_j, i, j);
    }

    void ConvertFrom(Point<T> * pts, size_t count) const
    {
      for (size_t i = 0; i < count; ++i)
        pts[i] = ConvertFrom(pts[i]);
    }

    Rect<T> const & GetLocalRect() const
    {
      return m_rect;
    }

    Rect<T> const GetGlobalRect() const
    {
      Point<T> pts[4];
      GetGlobalPoints(pts);

      Rect<T> res(pts[0], pts[1]);
      res.Add(pts[2]);
      res.Add(pts[3]);

      return res;
    }

    void GetGlobalPoints(Point<T> * pts) const
    {
      pts[0] = Point<T>(ConvertFrom(Point<T>(m_rect.minX(), m_rect.minY())));
      pts[1] = Point<T>(ConvertFrom(Point<T>(m_rect.minX(), m_rect.maxY())));
      pts[2] = Point<T>(ConvertFrom(Point<T>(m_rect.maxX(), m_rect.maxY())));
      pts[3] = Point<T>(ConvertFrom(Point<T>(m_rect.maxX(), m_rect.minY())));
    }

    template <typename U>
    void Inflate(U const & dx, U const & dy)
    {
      m_rect.Inflate(dx, dy);
    }

    void Add(AnyRect<T> const & r)
    {
      Point<T> pts[4];
      r.GetGlobalPoints(pts);
      ConvertTo(pts, 4);
      m_rect.Add(pts[0]);
      m_rect.Add(pts[1]);
      m_rect.Add(pts[2]);
      m_rect.Add(pts[3]);
    }

    void Offset(Point<T> const & p)
    {
      m_zero = ConvertTo(ConvertFrom(m_zero) + p);
    }

    Point<T> const Center() const
    {
      return ConvertFrom(m_rect.Center());
    }

    void SetSizesToIncludePoint(Point<T> const & p)
    {
      m_rect.SetSizesToIncludePoint(ConvertTo(p));
    }
  };

  template <typename T>
  AnyRect<T> const Offset(AnyRect<T> const & r, Point<T> const & pt)
  {
    AnyRect<T> res(r);
    res.Offset(pt);
    return res;
  }

  template <typename T, typename U>
  AnyRect<T> const Inflate(AnyRect<T> const & r, U const & dx, U const & dy)
  {
    AnyRect<T> res = r;
    res.Inflate(dx, dy);
    return res;
  }

  template <typename T, typename U>
  AnyRect<T> const Inflate(AnyRect<T> const & r, Point<U> const & pt)
  {
    return Inflate(r, pt.x, pt.y);
  }

  typedef AnyRect<double> AnyRectD;
  typedef AnyRect<float> AnyRectF;

  template <typename T>
  inline string DebugPrint(m2::AnyRect<T> const & r)
  {
    return DebugPrint(r.GetGlobalRect());
  }
}
