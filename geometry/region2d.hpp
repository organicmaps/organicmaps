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
    // Get a big type for storing middle calculations (x*y) to avoid overflow.
    template <int floating> struct BigType;
    template <> struct BigType<0> { typedef int64_t type; };
    template <> struct BigType<1> { typedef double type; };
  }

  template <class PointT>
  class Region
  {
    typedef vector<PointT> internal_container;

  public:
    typedef PointT value_type;
    typedef typename PointT::value_type coord_type;

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

    m2::Rect<coord_type> Rect() const { return m_rect; }

    bool IsValid() const { return m_points.size() > 2; }

  public:
    /// Taken from Computational Geometry in C and modified
    bool Contains(PointT const & pt) const
    {
      if (!m_rect.IsPointInside(pt))
        return false;

      int rCross = 0; /* number of right edge/ray crossings */
      int lCross = 0; /* number of left edge/ray crossings */

      size_t const numPoints = m_points.size();

      typedef typename detail::BigType<is_floating_point<coord_type>::value>::type BigCoordT;
      typedef Point<BigCoordT> BigPointT;

      BigPointT prev = BigPointT(m_points[numPoints - 1]) - BigPointT(pt);
      for (size_t i = 0; i < numPoints; ++i)
      {
        if (m_points[i] == pt)
          return true;

        BigPointT const curr = BigPointT(m_points[i]) - BigPointT(pt);

        bool const rCheck = ((curr.y > 0) != (prev.y > 0));
        bool const lCheck = ((curr.y < 0) != (prev.y < 0));

        if (rCheck || lCheck)
        {
          BigCoordT cp = CrossProduct(curr, prev);
          if (cp != 0)
          {
            ASSERT_NOT_EQUAL ( curr.y, prev.y, () );
            bool const PrevGreaterCurr = (prev.y > curr.y);

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

  private:
    internal_container m_points;
    m2::Rect<coord_type> m_rect;
  };

  typedef Region<m2::PointD> RegionD;
  typedef Region<m2::PointI> RegionI;
  typedef Region<m2::PointU> RegionU;
}
