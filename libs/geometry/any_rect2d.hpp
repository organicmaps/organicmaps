#pragma once

#include "geometry/angles.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/rect_intersect.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace m2
{
/// axis aligned rect
template <typename T>
class AnyRect
{
public:
  using Corners = std::array<Point<T>, 4>;

  AnyRect() = default;

  /// creating from regular rect
  explicit AnyRect(Rect<T> const & r)
  {
    if (r.IsValid())
    {
      m_zero = Point<T>(r.minX(), r.minY());
      m_rect = Rect<T>(0, 0, r.SizeX(), r.SizeY());
    }
    else
    {
      m_zero = Point<T>::Zero();
      m_rect = r;
    }
  }

  AnyRect(Point<T> const & zero, ang::Angle<T> const & angle, Rect<T> const & r) : m_angle(angle), m_rect(r)
  {
    m_zero = Convert(zero, Point<T>(1, 0), Point<T>(0, 1), i(), j());
  }

  Point<T> const & LocalZero() const { return m_zero; }

  Point<T> GlobalZero() const { return Convert(m_zero, i(), j(), Point<T>(1, 0), Point<T>(0, 1)); }

  Point<T> i() const { return Point<T>(m_angle.cos(), m_angle.sin()); }

  Point<T> j() const { return Point<T>(-m_angle.sin(), m_angle.cos()); }

  void SetAngle(ang::Angle<T> const & a)
  {
    Point<T> glbZero = GlobalZero();

    m_angle = a;
    m_zero = Convert(glbZero, Point<T>(1, 0), Point<T>(0, 1), i(), j());
  }

  ang::Angle<T> const & Angle() const { return m_angle; }

  Point<T> GlobalCenter() const { return ConvertFrom(m_rect.Center()); }

  Point<T> LocalCenter() const { return m_rect.Center(); }

  T GetMaxSize() const { return max(m_rect.SizeX(), m_rect.SizeY()); }

  bool EqualDxDy(AnyRect<T> const & r, T eps) const
  {
    Corners arr1;
    GetGlobalPoints(arr1);
    std::sort(arr1.begin(), arr1.end());

    Corners arr2;
    r.GetGlobalPoints(arr2);
    std::sort(arr2.begin(), arr2.end());

    for (size_t i = 0; i < arr1.size(); ++i)
      if (!arr1[i].EqualDxDy(arr2[i], eps))
        return false;

    return true;
  }

  bool IsPointInside(Point<T> const & pt) const { return m_rect.IsPointInside(ConvertTo(pt)); }

  bool IsRectInside(AnyRect<T> const & r) const
  {
    Corners pts;
    r.GetGlobalPoints(pts);
    ConvertTo(pts);
    return m_rect.IsPointInside(pts[0]) && m_rect.IsPointInside(pts[1]) && m_rect.IsPointInside(pts[2]) &&
           m_rect.IsPointInside(pts[3]);
  }

  bool IsIntersect(AnyRect<T> const & r) const
  {
    if (r.GetLocalRect() == Rect<T>())
      return false;
    Corners pts;
    r.GetGlobalPoints(pts);
    ConvertTo(pts);

    {
      Rect<T> r1;
      for (auto const & p : pts)
        r1.Add(p);

      if (!GetLocalRect().IsIntersect(r1))
        return false;
    }

    if (r.IsRectInside(*this))
      return true;

    if (IsRectInside(r))
      return true;

    return Intersect(GetLocalRect(), pts[0], pts[1]) || Intersect(GetLocalRect(), pts[1], pts[2]) ||
           Intersect(GetLocalRect(), pts[2], pts[3]) || Intersect(GetLocalRect(), pts[3], pts[0]);
  }

  /// Convert into coordinate system of this AnyRect
  Point<T> ConvertTo(Point<T> const & p) const
  {
    Point<T> i1(1, 0);
    Point<T> j1(0, 1);
    return Convert(p - Convert(m_zero, i(), j(), i1, j1), i1, j1, i(), j());
  }

  void ConvertTo(Corners & pts) const
  {
    for (auto & p : pts)
      p = ConvertTo(p);
  }

  /// Convert into global coordinates from the local coordinates of this AnyRect
  Point<T> ConvertFrom(Point<T> const & p) const
  {
    return Convert(p + m_zero, i(), j(), Point<T>(1, 0), Point<T>(0, 1));
  }

  Rect<T> const & GetLocalRect() const { return m_rect; }

  Rect<T> GetGlobalRect() const
  {
    Corners pts;
    GetGlobalPoints(pts);

    Rect<T> res;
    for (auto const & p : pts)
      res.Add(p);
    return res;
  }

  void GetGlobalPoints(Corners & pts) const
  {
    pts[0] = ConvertFrom(Point<T>(m_rect.minX(), m_rect.minY()));
    pts[1] = ConvertFrom(Point<T>(m_rect.minX(), m_rect.maxY()));
    pts[2] = ConvertFrom(Point<T>(m_rect.maxX(), m_rect.maxY()));
    pts[3] = ConvertFrom(Point<T>(m_rect.maxX(), m_rect.minY()));
  }

  template <typename U>
  void Inflate(U const & dx, U const & dy)
  {
    m_rect.Inflate(dx, dy);
  }

  void Add(AnyRect<T> const & r)
  {
    Corners pts;
    r.GetGlobalPoints(pts);
    ConvertTo(pts);
    for (auto const & p : pts)
      m_rect.Add(p);
  }

  void Offset(Point<T> const & p) { m_zero = ConvertTo(ConvertFrom(m_zero) + p); }

  Point<T> const Center() const { return ConvertFrom(m_rect.Center()); }

  void SetSizesToIncludePoint(Point<T> const & p) { m_rect.SetSizesToIncludePoint(ConvertTo(p)); }

  friend std::string DebugPrint(AnyRect<T> const & r)
  {
    return "{ Zero = " + DebugPrint(r.m_zero) + ", Rect = " + DebugPrint(r.m_rect) +
           ", Ang = " + DebugPrint(r.m_angle) + " }";
  }

private:
  static Point<T> Convert(Point<T> const & p, Point<T> const & fromI, Point<T> const & fromJ, Point<T> const & toI,
                          Point<T> const & toJ)
  {
    Point<T> res;

    res.x = p.x * DotProduct(fromI, toI) + p.y * DotProduct(fromJ, toI);
    res.y = p.x * DotProduct(fromI, toJ) + p.y * DotProduct(fromJ, toJ);

    return res;
  }

  ang::Angle<T> m_angle;

  Point<T> m_zero{};
  Rect<T> m_rect{};
};

using AnyRectD = AnyRect<double>;
using AnyRectF = AnyRect<float>;

template <typename T>
AnyRect<T> Offset(AnyRect<T> const & r, Point<T> const & pt)
{
  AnyRect<T> res(r);
  res.Offset(pt);
  return res;
}

template <typename T, typename U>
AnyRect<T> Inflate(AnyRect<T> const & r, U const & dx, U const & dy)
{
  AnyRect<T> res = r;
  res.Inflate(dx, dy);
  return res;
}

template <typename T, typename U>
AnyRect<T> Inflate(AnyRect<T> const & r, Point<U> const & pt)
{
  return Inflate(r, pt.x, pt.y);
}
}  // namespace m2
