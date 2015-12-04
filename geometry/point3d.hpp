#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/math.hpp"
#include "base/matrix.hpp"

#include "std/array.hpp"
#include "std/cmath.hpp"
#include "std/functional.hpp"
#include "std/sstream.hpp"
#include "std/typeinfo.hpp"


namespace m3
{
  template <typename T>
  class Point
  {
  public:
    typedef T value_type;

    T x, y, z;

    Point() {}
    Point(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
    template <typename U> Point(Point<U> const & u) : x(u.x), y(u.y), z(u.z) {}

    static Point<T> Zero() { return Point<T>(0, 0, 0, 0); }

    bool Equal(Point<T> const & p, T eps) const
    {
      return ((fabs(x - p.x) < eps) && (fabs(y - p.y) < eps) && (fabs(z - p.z) < eps));
    }

    T SquareLength(Point<T> const & p) const
    {
      return math::sqr(x- p.x) + math::sqr(y - p.y) + math::sqr(z - p.z);
    }

    double Length(Point<T> const & p) const
    {
      return sqrt(SquareLength(p));
    }

    Point<T> MoveXY(T len, T ang) const
    {
      return Point<T>(x + len * cos(ang), y + len * sin(ang), z, w);
    }

    Point<T> MoveXY(T len, T angSin, T angCos) const
    {
      return m3::Point<T>(x + len * angCos, y + len * angSin, z, w);
    }

    Point<T> const & operator-=(Point<T> const & a)
    {
      x -= a.x;
      y -= a.y;
      z -= a.z;
      return *this;
    }

    Point<T> const & operator+=(Point<T> const & a)
    {
      x += a.x;
      y += a.y;
      z += a.z;
      return *this;
    }

    template <typename U>
    Point<T> const & operator*=(U const & k)
    {
      x = static_cast<T>(x * k);
      y = static_cast<T>(y * k);
      z = static_cast<T>(z * k);
      return *this;
    }

    template <typename U>
    Point<T> const & operator=(Point<U> const & a)
    {
      x = static_cast<T>(a.x);
      y = static_cast<T>(a.y);
      z = static_cast<T>(a.z);
      return *this;
    }

    bool operator == (m3::Point<T> const & p) const
    {
      return x == p.x && y == p.y && z == p.z;
    }
    bool operator != (m3::Point<T> const & p) const
    {
      return !(*this == p);
    }
    m3::Point<T> operator + (m3::Point<T> const & pt) const
    {
      return m3::Point<T>(x + pt.x, y + pt.y, z + pt.z);
    }
    m3::Point<T> operator - (m3::Point<T> const & pt) const
    {
      return m3::Point<T>(x - pt.x, y - pt.y, z - pt.z);
    }
    m3::Point<T> operator -() const
    {
      return m3::Point<T>(-x, -y, -z);
    }

    m3::Point<T> operator * (T scale) const
    {
      return m3::Point<T>(x * scale, y * scale, z * scale);
    }

    m3::Point<T> const operator * (math::Matrix<T, 4, 4> const & m) const
    {
      math::Matrix<T, 1, 4> point = { x, y, z, 1};
      math::Matrix<T, 1, 4> res = point * m;
      return m3::Point<T>(res(0, 0) / res(0, 3), res(0, 1) / res(0, 3), res(0, 2) / res(0, 3));
    }

    m3::Point<T> operator / (T scale) const
    {
      return m3::Point<T>(x / scale, y / scale, z / scale);
    }

    m3::Point<T> mid(m3::Point<T> const & p) const
    {
      return m3::Point<T>((x + p.x) * 0.5, (y + p.y) * 0.5, (z + p.z) * 0.5);
    }

    /// @name VectorOperationsOnPoint
    // @{
    double Length() const
    {
      return sqrt(x*x + y*y + z*z);
    }

    Point<T> Normalize() const
    {
      ASSERT(!IsAlmostZero(), ());
      double const module = this->Length();
      return Point<T>(x / module, y / module, z / module);
    }
    // @}

    m3::Point<T> const & operator *= (math::Matrix<T, 4, 4> const & m)
    {
      math::Matrix<T, 1, 4> point = { x, y, z, 1};
      math::Matrix<T, 1, 4> res = point * m;
      x = res(0, 0) / res(0, 3);
      y = res(0, 1) / res(0, 3);
      z = res(0, 2) / res(0, 3);
      return *this;
    }

    void RotateXY(double angle)
    {
      T cosAngle = cos(angle);
      T sinAngle = sin(angle);
      T oldX = x;
      x = cosAngle * oldX - sinAngle * y;
      y = sinAngle * oldX + cosAngle * y;
    }
  };

  template <typename T>
  inline Point<T> const operator- (Point<T> const & a, Point<T> const & b)
  {
    return Point<T>(a.x - b.x, a.y - b.y, a.z - b.z);
  }

  template <typename T>
  inline Point<T> const operator+ (Point<T> const & a, Point<T> const & b)
  {
    return Point<T>(a.x + b.x, a.y + b.y, a.x + b.z);
  }

  template<typename T>
  Point<T> const Cross(Point<T> const & pt1, Point<T> const & pt2)
  {
    Point<T> const res(pt1.y * pt2.z - pt1.z * pt2.y,
                 pt1.z * pt2.x - pt1.x * pt2.z,
                 pt1.x * pt2.y - pt1.y * pt2.x;
    return res;
  }

  // Dot product of a and b, equals to |a|*|b|*cos(angle_between_a_and_b).
  template <typename T>
  T const DotProduct(Point<T> const & a, Point<T> const & b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  // Value of cross product of a and b, equals to |a|*|b|*sin(angle_between_a_and_b).
  template <typename T>
  T const CrossProduct(Point<T> const & a, Point<T> const & b)
  {
    return Cross(a, b).Length();
  }

  template <typename T, typename U>
  Point<T> const Shift(Point<T> const & pt, U const & dx, U const & dy, U const & dz)
  {
    return Point<T>(pt.x + dx, pt.y + dy, pt.z + dz);
  }

  template <typename T, typename U>
  Point<T> const Shift(Point<T> const & pt, Point<U> const & offset)
  {
    return Shift(pt, offset.x, offset.y, offset.z);
  }

  template <typename T>
  Point<T> const Floor(Point<T> const & pt)
  {
    Point<T> res;
    res.x = floor(pt.x);
    res.y = floor(pt.y);
    res.z = floor(pt.z);
    return res;
  }

  template <typename PointT>
  int GetOrientation(PointT const & p1, PointT const & p2, PointT const & pt)
  {
    double const sa = CrossProduct(p1 - pt, p2 - pt);
    if (sa > 0.0)
      return 1;
    if (sa < 0.0)
      return -1;
    return 0;
  }

  template <typename T> string DebugPrint(m3::Point<T> const & p)
  {
    ostringstream out;
    out.precision(20);
    out << "m3::Point<" << typeid(T).name() << ">(" << p.x << ", " << p.y << ", " << p.z << ")";
    return out.str();
  }

  template <class TArchive, class PointT>
  TArchive & operator >> (TArchive & ar, m3::Point<PointT> & pt)
  {
    ar >> pt.x;
    ar >> pt.y;
    ar >> pt.z;
    return ar;
  }

  template <class TArchive, class PointT>
  TArchive & operator << (TArchive & ar, m3::Point<PointT> const & pt)
  {
    ar << pt.x;
    ar << pt.y;
    ar << pt.z;
    return ar;
  }

  template <typename T>
  bool operator< (Point<T> const & l, Point<T> const & r)
  {
    if (l.x != r.x)
      return l.x < r.x;
    if (l.y != r.y)
      return l.y < r.y;
    return l.z < r.z;
  }

  typedef Point<float> PointF;
  typedef Point<double> PointD;
  typedef Point<uint32_t> PointU;
  typedef Point<uint64_t> PointU64;
  typedef Point<int32_t> PointI;
  typedef Point<int64_t> PointI64;
}

