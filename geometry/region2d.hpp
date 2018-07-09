#pragma once

#include "geometry/distance.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/math.hpp"

#include "std/algorithm.hpp"
#include "std/type_traits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace m2
{
  namespace detail
  {
    struct DefEqualFloat
    {
      // 1e-9 is two orders of magnitude more accurate than our OSM source data.
      static double constexpr kPrecision = 1e-9;

      template <class TPoint>
      bool EqualPoints(TPoint const & p1, TPoint const & p2) const
      {
        static_assert(std::is_floating_point<typename TPoint::value_type>::value, "");

        return my::AlmostEqualAbs(p1.x, p2.x, static_cast<typename TPoint::value_type>(kPrecision)) &&
               my::AlmostEqualAbs(p1.y, p2.y, static_cast<typename TPoint::value_type>(kPrecision));
      }
      template <class TCoord>
      bool EqualZeroSquarePrecision(TCoord val) const
      {
        static_assert(std::is_floating_point<TCoord>::value, "");

        return my::AlmostEqualAbs(val, 0.0, kPrecision * kPrecision);
      }
      // Determines if value of a val lays between a p1 and a p2 values with some precision.
      inline bool IsAlmostBetween(double val, double p1, double p2) const
      {
        return (val >= p1 - kPrecision && val <= p2 + kPrecision) ||
               (val <= p1 + kPrecision && val >= p2 - kPrecision);
      }
    };

    struct DefEqualInt
    {
      template <class TPoint>
      bool EqualPoints(TPoint const & p1, TPoint const & p2) const
      {
        return p1 == p2;
      }
      template <class TCoord>
      bool EqualZeroSquarePrecision(TCoord val) const
      {
        return val == 0;
      }
      inline bool IsAlmostBetween(double val, double left, double right) const
      {
        return (val >= left && val <= right) ||
               (val <= left && val >= right);
      }
    };

    template <int floating> struct TraitsType;
    template <> struct TraitsType<1>
    {
      typedef DefEqualFloat EqualType;
      typedef double BigType;
    };
    template <> struct TraitsType<0>
    {
      typedef DefEqualInt EqualType;
      typedef int64_t BigType;
    };
  }

  template <class PointT>
  class Region
  {
    template <class TArchive, class TPoint>
    friend TArchive & operator << (TArchive & ar, Region<TPoint> const & region);
    template <class TArchive, class TPoint>
    friend TArchive & operator >> (TArchive & ar, Region<TPoint> & region);

  public:
    typedef PointT ValueT;
    typedef typename PointT::value_type CoordT;

  private:
    typedef vector<PointT> ContainerT;
    typedef detail::TraitsType<is_floating_point<CoordT>::value> TraitsT;

  public:
    /// @name Needed for boost region concept.
    //@{
    typedef typename ContainerT::const_iterator IteratorT;
    IteratorT Begin() const { return m_points.begin(); }
    IteratorT End() const { return m_points.end(); }
    size_t Size() const { return m_points.size(); }
    //@}

  public:
    Region() = default;

    template <class Points>
    explicit Region(Points && points) : m_points(std::forward<Points>(points))
    {
      CalcLimitRect();
    }

    template <class IterT>
    Region(IterT first, IterT last) : m_points(first, last)
    {
      CalcLimitRect();
    }

    template <class IterT>
    void Assign(IterT first, IterT last)
    {
      m_points.assign(first, last);
      CalcLimitRect();
    }

    template <class IterT, class Fn>
    void AssignEx(IterT first, IterT last, Fn fn)
    {
      m_points.reserve(distance(first, last));

      while (first != last)
        m_points.push_back(fn(*first++));

      CalcLimitRect();
    }

    void AddPoint(PointT const & pt)
    {
      m_points.push_back(pt);
      m_rect.Add(pt);
    }

    template <class TFunctor>
    void ForEachPoint(TFunctor toDo) const
    {
      for_each(m_points.begin(), m_points.end(), toDo);
    }

    inline m2::Rect<CoordT> const & GetRect() const { return m_rect; }
    inline size_t GetPointsCount() const { return m_points.size(); }
    inline bool IsValid() const { return GetPointsCount() > 2; }

    void Swap(Region<PointT> & rhs)
    {
      m_points.swap(rhs.m_points);
      std::swap(m_rect, rhs.m_rect);
    }

    ContainerT const & Data() const { return m_points; }

    template <class TEqualF>
    static inline bool IsIntersect(CoordT const & x11, CoordT const & y11, CoordT const & x12, CoordT const & y12,
                            CoordT const & x21, CoordT const & y21, CoordT const & x22, CoordT const & y22,
                            TEqualF equalF, PointT & pt)
    {
      double const divider = ((y12 - y11) * (x22 - x21) - (x12 - x11) * (y22-y21));
      if (equalF.EqualZeroSquarePrecision(divider))
        return false;
      double v = ((x12 - x11) * (y21 - y11) + (y12 - y11) * (x11 - x21)) / divider;
      PointT p(x21 + (x22 - x21) * v, y21 + (y22 - y21) * v);

      if (!equalF.IsAlmostBetween(p.x, x11, x12))
        return false;
      if (!equalF.IsAlmostBetween(p.x, x21, x22))
        return false;
      if (!equalF.IsAlmostBetween(p.y, y11, y12))
        return false;
      if (!equalF.IsAlmostBetween(p.y, y21, y22))
        return false;

      pt = p;
      return true;
    }

    static inline bool IsIntersect(PointT const & p1, PointT const & p2, PointT const & p3, PointT const & p4 , PointT & pt)
    {
      return IsIntersect(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, typename TraitsT::EqualType(), pt);
    }

  public:

    /// Taken from Computational Geometry in C and modified
    template <class TEqualF>
    bool Contains(PointT const & pt, TEqualF equalF) const
    {
      if (!m_rect.IsPointInside(pt))
        return false;

      int rCross = 0; /* number of right edge/ray crossings */
      int lCross = 0; /* number of left edge/ray crossings */

      size_t const numPoints = m_points.size();

      typedef typename TraitsT::BigType BigCoordT;
      typedef Point<BigCoordT> BigPointT;

      BigPointT prev = BigPointT(m_points[numPoints - 1]) - BigPointT(pt);
      for (size_t i = 0; i < numPoints; ++i)
      {
        if (equalF.EqualPoints(m_points[i], pt))
          return true;

        BigPointT const curr = BigPointT(m_points[i]) - BigPointT(pt);

        bool const rCheck = ((curr.y > 0) != (prev.y > 0));
        bool const lCheck = ((curr.y < 0) != (prev.y < 0));

        if (rCheck || lCheck)
        {
          ASSERT_NOT_EQUAL ( curr.y, prev.y, () );

          BigCoordT const delta = prev.y - curr.y;
          BigCoordT const cp = CrossProduct(curr, prev);

          // Squared precision is needed here because of comparison between cross product of two
          // vectors and zero. It's impossible to compare them relatively, so they're compared
          // absolutely, and, as cross product is proportional to product of lengths of both
          // operands precision must be squared too.
          if (!equalF.EqualZeroSquarePrecision(cp))
          {
            bool const PrevGreaterCurr = delta > 0.0;

            if (rCheck && ((cp > 0) == PrevGreaterCurr)) ++rCross;
            if (lCheck && ((cp > 0) != PrevGreaterCurr)) ++lCross;
          }
        }

        prev = curr;
      }

      /* q on the edge if left and right cross are not the same parity. */
      if ((rCross & 1) != (lCross & 1))
        return true;  // on the edge

      /* q inside if an odd number of crossings. */
      if (rCross & 1)
        return true;  // inside
      else
        return false; // outside
    }

    bool Contains(PointT const & pt) const
    {
      return Contains(pt, typename TraitsT::EqualType());
    }

    /// Finds point of intersection with the section.
    bool FindIntersection(PointT const & point1, PointT const & point2, PointT & result) const
    {
      if (m_points.empty())
        return false;
      PointT const * prev = &m_points.back();
      for (PointT const & curr : m_points)
      {
        if (IsIntersect(point1, point2, *prev, curr, result))
          return true;
        prev = &curr;
      }
      return false;
    }

    /// Slow check that point lies at the border.
    template <class TEqualF>
    bool AtBorder(PointT const & pt, double const delta, TEqualF equalF) const
    {
      if (!m_rect.IsPointInside(pt))
        return false;

      const double squareDelta = delta*delta;
      size_t const numPoints = m_points.size();

      PointT prev = m_points[numPoints - 1];
      DistanceToLineSquare<PointT> distance;
      for (size_t i = 0; i < numPoints; ++i)
      {
        PointT const curr = m_points[i];

        // Borders often have same points with ways
        if (equalF.EqualPoints(m_points[i], pt))
          return true;

        distance.SetBounds(prev, curr);
        if (distance(pt) < squareDelta)
          return true;

        prev = curr;
      }

      return false; // Point lies outside the border.
    }

    bool AtBorder(PointT const & pt, double const delta) const
    {
      return AtBorder(pt, delta, typename TraitsT::EqualType());
    }

  private:
    void CalcLimitRect()
    {
      m_rect.MakeEmpty();
      for (size_t i = 0; i < m_points.size(); ++i)
        m_rect.Add(m_points[i]);
    }

    ContainerT m_points;
    m2::Rect<CoordT> m_rect;

    template <class T> friend string DebugPrint(Region<T> const &);
  };

  template <class PointT>
  void swap(Region<PointT> & r1, Region<PointT> & r2)
  {
    r1.Swap(r2);
  }

  template <class TArchive, class PointT>
  TArchive & operator >> (TArchive & ar, Region<PointT> & region)
  {
    ar >> region.m_rect;
    ar >> region.m_points;
    return ar;
  }

  template <class TArchive, class PointT>
  TArchive & operator << (TArchive & ar, Region<PointT> const & region)
  {
    ar << region.m_rect;
    ar << region.m_points;
    return ar;
  }

  template <class PointT>
  inline string DebugPrint(Region<PointT> const & r)
  {
    return (DebugPrint(r.m_rect) + ::DebugPrint(r.m_points));
  }

  template <class Point>
  bool RegionsContain(vector<Region<Point>> const & regions, Point const & point)
  {
    for (auto const & region : regions)
    {
      if (region.Contains(point))
        return true;
    }

    return false;
  }

  typedef Region<m2::PointD> RegionD;
  typedef Region<m2::PointI> RegionI;
  typedef Region<m2::PointU> RegionU;
}
