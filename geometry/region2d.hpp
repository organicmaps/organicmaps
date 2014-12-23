#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"
#include "distance.hpp"

#include "../std/vector.hpp"
#include "../std/algorithm.hpp"
#include "../std/type_traits.hpp"


namespace m2
{
  namespace detail
  {
    struct DefEqualFloat
    {
      template <class PointT>
      bool EqualPoints(PointT const & p1, PointT const & p2) const
      {
        return m2::AlmostEqual(p1, p2);
      }
      template <class CoordT>
      bool EqualZero(CoordT val, CoordT exp) const
      {
        return my::AlmostEqual(val + exp, exp);
      }
    };

    struct DefEqualInt
    {
      template <class PointT>
      bool EqualPoints(PointT const & p1, PointT const & p2) const
      {
        return p1 == p2;
      }
      template <class CoordT>
      bool EqualZero(CoordT val, CoordT) const
      {
        return val == 0;
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
    Region() {}

    template <class IterT>
    Region(IterT first, IterT last)
      : m_points(first, last)
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

    inline m2::Rect<CoordT> GetRect() const { return m_rect; }
    inline size_t GetPointsCount() const { return m_points.size(); }
    inline bool IsValid() const { return GetPointsCount() > 2; }

    void Swap(Region<PointT> & rhs)
    {
      m_points.swap(rhs.m_points);
      std::swap(m_rect, rhs.m_rect);
    }

    ContainerT Data() const { return m_points; }

  public:
    /// Taken from Computational Geometry in C and modified
    template <class EqualF>
    bool Contains(PointT const & pt, EqualF equalF) const
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

          if (!equalF.EqualZero(cp, delta))
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

    /// Slow point at border realisation
    template <class EqualF>
    bool atBorder(PointT const & pt, double const delta, EqualF equalF) const
    {
      if (!m_rect.IsPointInside(pt))
        return false;

      const double squareDelta = delta*delta;

      size_t const numPoints = m_points.size();

      typedef typename TraitsT::BigType BigCoordT;
      typedef Point<BigCoordT> BigPointT;

      PointT prev = m_points[numPoints - 1];
      DistanceToLineSquare<PointT> distance;
      for (size_t i = 0; i < numPoints; ++i)
      {
        PointT const curr = m_points[i];

        // Borders often has same points with ways
        if (equalF.EqualPoints(m_points[i], pt))
          return true;

        distance.SetBounds(prev, curr);
        if (distance(pt) < squareDelta)
          return true;

        prev = curr;
      }

        return false; // outside
    }

    bool atBorder(PointT const & pt, double const delta) const
    {
      return atBorder(pt, delta, typename TraitsT::EqualType());
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

  typedef Region<m2::PointD> RegionD;
  typedef Region<m2::PointI> RegionI;
  typedef Region<m2::PointU> RegionU;
}
