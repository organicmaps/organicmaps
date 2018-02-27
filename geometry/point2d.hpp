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


namespace m2
{
  template <typename T>
  class Point
  {
  public:
    typedef T value_type;

    T x, y;

    Point() : x(T()), y(T()) {}
    Point(T x_, T y_) : x(x_), y(y_) {}
    template <typename U> Point(Point<U> const & u) : x(u.x), y(u.y) {}

    static Point<T> Zero() { return Point<T>(0, 0); }

    bool EqualDxDy(Point<T> const & p, T eps) const
    {
      return ((fabs(x - p.x) < eps) && (fabs(y - p.y) < eps));
    }

    // TODO (@y, @m): rename to SquaredLength.
    T SquareLength(Point<T> const & p) const { return pow(x - p.x, 2) + pow(y - p.y, 2); }

    double Length(Point<T> const & p) const
    {
      return sqrt(SquareLength(p));
    }

    bool IsAlmostZero() const
    {
      return AlmostEqualULPs(*this, Point<T>(0,0));
    }

    Point<T> Move(T len, T ang) const
    {
      return Point<T>(x + len * cos(ang), y + len * sin(ang));
    }

    Point<T> Move(T len, T angSin, T angCos) const
    {
      return m2::Point<T>(x + len * angCos, y + len * angSin);
    }

    Point<T> const & operator-=(Point<T> const & a)
    {
      x -= a.x;
      y -= a.y;
      return *this;
    }

    Point<T> const & operator+=(Point<T> const & a)
    {
      x += a.x;
      y += a.y;
      return *this;
    }

    template <typename U>
    Point<T> const & operator*=(U const & k)
    {
      x = static_cast<T>(x * k);
      y = static_cast<T>(y * k);
      return *this;
    }

    template <typename U>
    Point<T> const & operator=(Point<U> const & a)
    {
      x = static_cast<T>(a.x);
      y = static_cast<T>(a.y);
      return *this;
    }

    bool operator == (m2::Point<T> const & p) const
    {
      return x == p.x && y == p.y;
    }
    bool operator != (m2::Point<T> const & p) const
    {
      return !(*this == p);
    }
    m2::Point<T> operator + (m2::Point<T> const & pt) const
    {
      return m2::Point<T>(x + pt.x, y + pt.y);
    }
    m2::Point<T> operator - (m2::Point<T> const & pt) const
    {
      return m2::Point<T>(x - pt.x, y - pt.y);
    }
    m2::Point<T> operator -() const
    {
      return m2::Point<T>(-x, -y);
    }

    m2::Point<T> operator * (T scale) const
    {
      return m2::Point<T>(x * scale, y * scale);
    }

    m2::Point<T> const operator * (math::Matrix<T, 3, 3> const & m) const
    {
      m2::Point<T> res;
      res.x = x * m(0, 0) + y * m(1, 0) + m(2, 0);
      res.y = x * m(0, 1) + y * m(1, 1) + m(2, 1);
      return res;
    }

    m2::Point<T> operator / (T scale) const
    {
      return m2::Point<T>(x / scale, y / scale);
    }

    m2::Point<T> mid(m2::Point<T> const & p) const
    {
      return m2::Point<T>((x + p.x) * 0.5, (y + p.y) * 0.5);
    }

    T SquaredLength() const { return x * x + y * y; }

    /// @name VectorOperationsOnPoint
    // @{
    double Length() const
    {
      return sqrt(SquaredLength());
    }

    Point<T> Normalize() const
    {
      ASSERT(!IsAlmostZero(), ());
      double const module = this->Length();
      return Point<T>(x / module, y / module);
    }

    std::pair<Point<T>, Point<T> > Normals(T prolongationFactor = 1) const
    {
      T const prolongatedX = prolongationFactor * x;
      T const prolongatedY = prolongationFactor * y;
      return std::pair<Point<T>, Point<T> >(Point<T>(static_cast<T>(-prolongatedY), static_cast<T>(prolongatedX)),
                                       Point<T>(static_cast<T>(prolongatedY), static_cast<T>(-prolongatedX)));
    }
    // @}

    m2::Point<T> const & operator *= (math::Matrix<T, 3, 3> const & m)
    {
      T tempX = x;
      x = tempX * m(0, 0) + y * m(1, 0) + m(2, 0);
      y = tempX * m(0, 1) + y * m(1, 1) + m(2, 1);
      return *this;
    }

    void Rotate(double angle)
    {
      T cosAngle = cos(angle);
      T sinAngle = sin(angle);
      T oldX = x;
      x = cosAngle * oldX - sinAngle * y;
      y = sinAngle * oldX + cosAngle * y;
    }

    // Returns vector rotated 90 degrees counterclockwise.
    Point Ort() const { return Point(-y, x); }

    void Transform(m2::Point<T> const & org,
                   m2::Point<T> const & dx, m2::Point<T> const & dy)
    {
      T oldX = x;
      x = org.x + oldX * dx.x + y * dy.x;
      y = org.y + oldX * dx.y + y * dy.y;
    }

    struct Hash
    {
      size_t operator()(m2::Point<T> const & p) const
      {
        return my::Hash(p.x, p.y);
      }
    };
  };

  template <typename T>
  inline Point<T> const operator- (Point<T> const & a, Point<T> const & b)
  {
    return Point<T>(a.x - b.x, a.y - b.y);
  }

  template <typename T>
  inline Point<T> const operator+ (Point<T> const & a, Point<T> const & b)
  {
    return Point<T>(a.x + b.x, a.y + b.y);
  }

  // Dot product of a and b, equals to |a|*|b|*cos(angle_between_a_and_b).
  template <typename T>
  T const DotProduct(Point<T> const & a, Point<T> const & b)
  {
    return a.x * b.x + a.y * b.y;
  }

  // Value of cross product of a and b, equals to |a|*|b|*sin(angle_between_a_and_b).
  template <typename T>
  T const CrossProduct(Point<T> const & a, Point<T> const & b)
  {
    return a.x * b.y - a.y * b.x;
  }

  template <typename T>
  Point<T> const Rotate(Point<T> const & pt, T a)
  {
    Point<T> res(pt);
    res.Rotate(a);
    return res;
  }

  template <typename T, typename U>
  Point<T> const Shift(Point<T> const & pt, U const & dx, U const & dy)
  {
    return Point<T>(pt.x + dx, pt.y + dy);
  }

  template <typename T, typename U>
  Point<T> const Shift(Point<T> const & pt, Point<U> const & offset)
  {
    return Shift(pt, offset.x, offset.y);
  }

  template <typename T>
  Point<T> const Floor(Point<T> const & pt)
  {
    Point<T> res;
    res.x = floor(pt.x);
    res.y = floor(pt.y);
    return res;
  }

  template <typename T> std::string DebugPrint(m2::Point<T> const & p)
  {
    ostringstream out;
    out.precision(20);
    out << "m2::Point<" << typeid(T).name() << ">(" << p.x << ", " << p.y << ")";
    return out.str();
  }

  template <typename T>
  bool AlmostEqualAbs(m2::Point<T> const & a, m2::Point<T> const & b, double const eps)
  {
    return my::AlmostEqualAbs(a.x, b.x, eps) && my::AlmostEqualAbs(a.y, b.y, eps);
  }

  template <typename T>
  bool AlmostEqualULPs(m2::Point<T> const & a, m2::Point<T> const & b, unsigned int maxULPs = 256)
  {
    return my::AlmostEqualULPs(a.x, b.x, maxULPs) && my::AlmostEqualULPs(a.y, b.y, maxULPs);
  }

  /// Calculate three points of a triangle (p1, p2 and p3) which give an arrow that
  /// presents an equilateral triangle with the median
  /// starting at point b and having direction b,e.
  /// The height of the equilateral triangle is l and the base of the triangle is 2 * w
  template <typename T, typename TT, typename PointT = Point<T>>
  void GetArrowPoints(PointT const & b, PointT const & e, T w, T l, array<Point<TT>, 3> & arrPnts)
  {
    ASSERT(!m2::AlmostEqualULPs(b, e), ());

    PointT const beVec = e - b;
    PointT beNormalizedVec = beVec.Normalize();
    std::pair<PointT, PointT > beNormVecs = beNormalizedVec.Normals(w);

    arrPnts[0] = e + beNormVecs.first;
    arrPnts[1] = e + beNormalizedVec * l;
    arrPnts[2] = e + beNormVecs.second;
  }

  /// Returns a point which is belonged to the segment p1, p2 with respet the indent shiftFromP1 from p1.
  /// If shiftFromP1 is more the distance between (p1, p2) it returns p2.
  /// If shiftFromP1 is less or equal zero it returns p1.
  template <typename T>
  Point<T> PointAtSegment(Point<T> const & p1, Point<T> const & p2, T shiftFromP1)
  {
    Point<T> p12 = p2 - p1;
    shiftFromP1 = my::clamp(shiftFromP1, static_cast<T>(0.0), static_cast<T>(p12.Length()));
    return p1 + p12.Normalize() * shiftFromP1;
  }

  template <class TArchive, class PointT>
  TArchive & operator >> (TArchive & ar, m2::Point<PointT> & pt)
  {
    ar >> pt.x;
    ar >> pt.y;
    return ar;
  }

  template <class TArchive, class PointT>
  TArchive & operator << (TArchive & ar, m2::Point<PointT> const & pt)
  {
    ar << pt.x;
    ar << pt.y;
    return ar;
  }

  template <typename T>
  bool operator< (Point<T> const & l, Point<T> const & r)
  {
    if (l.x != r.x)
      return l.x < r.x;
    return l.y < r.y;
  }

  typedef Point<float> PointF;
  typedef Point<double> PointD;
  typedef Point<uint32_t> PointU;
  typedef Point<uint64_t> PointU64;
  typedef Point<int32_t> PointI;
  typedef Point<int64_t> PointI64;
}  // namespace m2

namespace my
{

template <typename T>
bool AlmostEqualULPs(m2::Point<T> const & p1, m2::Point<T> const & p2, unsigned int maxULPs = 256)
{
  return m2::AlmostEqualULPs(p1, p2, maxULPs);
}

template <typename T>
bool AlmostEqualAbs(m2::Point<T> const & p1, m2::Point<T> const & p2, double const & eps)
{
  return m2::AlmostEqualAbs(p1, p2, eps);
}

}  // namespace my
