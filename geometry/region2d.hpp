#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"

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
    typedef PointT value_type;
    typedef typename PointT::value_type coord_type;

  private:
    typedef vector<PointT> internal_container;
    typedef detail::TraitsType<is_floating_point<coord_type>::value> traits_type;

  public:
    Region() {}

    template <class TInputIterator>
    Region(TInputIterator first, TInputIterator last)
      : m_points(first, last)
    {
      // update limit rect
      for (; first != last; ++first)
        m_rect.Add(*first);
    }

    template <class TInputIterator>
    void Assign(TInputIterator first, TInputIterator last)
    {
      m_points.assign(first, last);
      m_rect.MakeEmpty();
      for (; first != last; ++first)
        m_rect.Add(*first);
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

    m2::Rect<coord_type> GetRect() const { return m_rect; }

    bool IsValid() const { return m_points.size() > 2; }

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

      typedef typename traits_type::BigType BigCoordT;
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

            if (rCheck && (cp > 0 == PrevGreaterCurr)) ++rCross;
            if (lCheck && (cp > 0 != PrevGreaterCurr)) ++lCross;
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
      return Contains(pt, typename traits_type::EqualType());
    }

  private:
    internal_container m_points;
    m2::Rect<coord_type> m_rect;
  };

  template <class TArchive, class PointT>
  TArchive & operator >> (TArchive & ar, m2::Region<PointT> & region)
  {
    ar >> region.m_rect;
    ar >> region.m_points;
    return ar;
  }

  template <class TArchive, class PointT>
  TArchive & operator << (TArchive & ar, m2::Region<PointT> const & region)
  {
    ar << region.m_rect;
    ar << region.m_points;
    return ar;
  }

  typedef Region<m2::PointD> RegionD;
  typedef Region<m2::PointI> RegionI;
  typedef Region<m2::PointU> RegionU;
}
