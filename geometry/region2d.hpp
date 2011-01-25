#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"

#include "../std/vector.hpp"
#include "../std/algorithm.hpp"
#include "../std/limits.hpp"
#include "../std/static_assert.hpp"

namespace m2
{
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
    void ForEachPoint(TFunctor & f) const
    {
      for (typename internal_container::const_iterator it = m_points.begin();
            it != m_points.end(); ++it)
        f(*it);
    }

    m2::Rect<coord_type> Rect() const { return m_rect; }

    bool IsValid() const { return m_points.size() > 2; }

  private:
    class cp_calc
    {
      bool m_calc;
      bool m_cpG0;
      bool m_pGc;

    public:
      cp_calc() : m_calc(true) {}

      void calc(PointT const & curr, PointT const & prev)
      {
        if (m_calc)
        {
          ASSERT_NOT_EQUAL ( curr.y, prev.y, () );

          m_calc = false;
          m_cpG0 = CrossProduct(curr, prev) > 0;
          m_pGc = (prev.y > curr.y);
        }
      }

      bool equal() const { return m_cpG0 == m_pGc; }
      bool not_equal() const { return m_cpG0 != m_pGc; }
    };

  public:
    /// Taken from Computational Geometry in C and modified
    bool Contains(PointT const pt) const
    {
      if (!IsValid() || !m_rect.IsPointInside(pt))
        return false;

      int rCross = 0; /* number of right edge/ray crossings */
      int lCross = 0; /* number of left edge/ray crossings */

      size_t const numPoints = m_points.size();

      size_t i1 = numPoints - 1;
      /* For each edge e=(i-1,i), see if crosses ray. */
      for (size_t i = 0; i < numPoints; ++i)
      {
        /* First see if q=(0,0) is a vertex. */
        if (m_points[i] == pt)
          return true;  // vertex

        PointT const curr = m_points[i] - pt;
        PointT const prev = m_points[i1] - pt;

        cp_calc calc;
        if ((curr.y > 0) != (prev.y > 0))
        {
          /* crosses ray if strictly positive intersection. */
          calc.calc(curr, prev);
          if (calc.equal())
            ++rCross;
        }

        if ((curr.y < 0) != (prev.y < 0))
        {
          /* crosses ray if strictly positive intersection. */
          calc.calc(curr, prev);
          if (calc.not_equal())
            ++lCross;
        }

        i1 = i;
      }

      /* q on the edge if left and right cross are not the same parity. */
      if ((rCross & 1) != (lCross & 1))
        return true;  // on the edge

      /* q inside iff an odd number of crossings. */
      if (rCross & 1)
        return true;  // inside
      else
        return false; // outside
    }

//    bool Contains(PointT const pt, bool pointOnBorder = true) const
//    {
//      if (!IsValid() || !m_rect.IsPointInside(pt))
//        return false;

//      bool isInside = false;

//      //left vertex
//      PointT p1 = m_points[0];

//      size_t const size = m_points.size();
//      //check all rays
//      for (size_t i = 1; i <= size; ++i)
//      {
//        //point is an vertex
//        if (pt == p1)
//          return pointOnBorder;

//        //right vertex
//        PointT const p2 = m_points[i % size];

//        //ray is outside of our interests
//        if (pt.y < min(p1.y, p2.y) || pt.y > max(p1.y, p2.y))
//        {
//          //next ray left point
//          p1 = p2;
//          continue;
//        }

//        //ray is crossing over by the algorithm (common part of)
//        if (pt.y > min(p1.y, p2.y) && pt.y < max(p1.y, p2.y))
//        {
//            //x is before of ray
//            if (pt.x <= max(p1.x, p2.x))
//            {
//              //overlies on a horizontal ray
//              if (p1.y == p2.y && pt.x >= min(p1.x, p2.x))
//                return pointOnBorder;

//              //ray is vertical
//              if (p1.x == p2.x)
//              {
//                //overlies on a ray
//                if (p1.x == pt.x)
//                  return pointOnBorder;
//                //before ray
//                else
//                  isInside = !isInside;
//              }
//              else //cross point on the left side
//              {
//                //cross point of x
//                double const xinters = (pt.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;

//                //overlies on a ray
//                if (abs(pt.x - xinters) < __DBL_EPSILON__)
//                  return pointOnBorder;

//                //before ray
//                if (pt.x < xinters)
//                  isInside = !isInside;
//              }
//            }
//          }
//          //special case when ray is crossing through the vertex
//          else
//          {
//            //p crossing over p2
//            if (pt.y == p2.y && pt.x <= p2.x)
//            {
//              //next vertex
//              PointT const p3 = m_points[(i+1) % size];

//              //p.y lies between p1.y & p3.y
//              if (pt.y >= min(p1.y, p3.y) && pt.y <= max(p1.y, p3.y))
//                isInside = !isInside;
//            }
//          }

//          //next ray left point
//          p1 = p2;
//      }
//      return isInside;
//    }

//    bool Contains(TPoint const pt) const
//    {
//      // quick and dirty intersect
//      if (!IsValid() || !m_rect.IsPointInside(pt))
//        return false;

//      bool isInside = false;
//      size_t const size = m_points.size();
//      for (size_t i = 0; i < size; ++i)
//      {
//        isInside = false;
//        size_t i1 = i < size - 1 ? i + 1 : 0;
//        while (!isInside)
//        {
//         size_t i2 = i1 + 1;
//         if (i2 >= size)
//           i2 = 0;
//         if (i2 == (i < size - 1 ? i + 1 : 0))
//           break;
//         size_t S = abs(m_points[i1].x * (m_points[i2].y - m_points[i].y) +
//            m_points[i2].x * (m_points[i].y - m_points[i1].y) +
//            m_points[i].x  * (m_points[i1].y - m_points[i2].y));
//         size_t S1 = abs(m_points[i1].x * (m_points[i2].y - pt.y) +
//            m_points[i2].x * (pt.y - m_points[i1].y) +
//            pt.x * (m_points[i1].y - m_points[i2].y));
//         size_t S2 = abs(m_points[i].x * (m_points[i2].y - pt.y) +
//            m_points[i2].x * (pt.y - m_points[i].y) +
//            pt.x * (m_points[i].y - m_points[i2].y));
//         size_t S3 = abs(m_points[i1].x * (m_points[i].y - pt.y) +
//            m_points[i].x * (pt.y - m_points[i1].y) +
//            pt.x * (m_points[i1].y - m_points[i].y));
//         if (S == S1 + S2 + S3)
//         {
//          isInside = true;
//          break;
//         }
//         i1 = i1 + 1;
//         if (i1 >= size)
//           i1 = 0;
//        }
//        if (!isInside)
//          break;
//       }
//       return isInside;
//    }

//    bool Contains(TPoint const & pt) const
//    {
//      if (!IsValid() || !m_rect.IsPointInside(pt))
//        return false;

//      size_t const size = m_points.size();
//      size_t j = size - 1;
//      bool isInside = false;

//      for (size_t i = 0; i < size; ++i)
//      {
//        // special case when point is exactly on vertex
//        if (pt == m_points[i])
//          return true;

//        if (m_points[i].y < pt.y && m_points[j].y >= pt.y
//            || m_points[j].y < pt.y && m_points[i].y >= pt.y)
//        {
//          if (m_points[i].x + (pt.y - m_points[i].y) /
//              (m_points[j].y - m_points[i].y) * (m_points[j].x - m_points[i].x) < pt.x)
//            isInside = !isInside;
//        }
//        j = i;
//      }

//      return isInside ;
//    }

//    bool Contains(TPoint const & pt) const
//    {
//      // Raycast point in polygon method
//      size_t const size = m_points.size();
//      bool inPoly = false;
//      size_t i;
//      size_t j = size - 1;

//      for (i = 0; i < size; ++i)
//      {
//        TPoint vertex1 = m_points[i];
//        TPoint vertex2 = m_points[j];

//        if ((vertex1.x < pt.x == vertex2.x >= pt.x) || vertex2.x < pt.x && vertex1.x >= pt.x)
//        {
//          if (vertex1.y + (pt.x - vertex1.x) / (vertex2.x - vertex1.x)
//              * (vertex2.y - vertex1.y) < pt.y)
//          {
//            inPoly = !inPoly;
//          }
//        }

//        j = i;
//      }

//      return inPoly;
//    }

//    bool Contains(PointT const & pt) const
//    {
//      size_t const size = m_points.size();
//      bool isInside = false;
//      for (size_t i = 0, j = size - 1; i < size; j = i++)
//      {
//        if (m_points[i] == pt)
//          return true;  // vertex itself

//        if ( ((m_points[i].y > pt.y) != (m_points[j].y > pt.y)) &&
//            (pt.x < (m_points[j].x - m_points[i].x)
//             * (pt.y - m_points[i].y) / (m_points[j].y - m_points[i].y) + m_points[i].x) )
//          isInside = !isInside;
//      }
//      return isInside;
//    }

//    bool Contains(TPoint const & point) const
//    {
//      size_t const size = m_points.size();
//      if (size < 3)
//        return false;

//      bool isInside = false;
//      TPoint old = m_points[size - 1];
//      TPoint p1, p2;
//      for (size_t i = 0; i < size - 1; ++i)
//      {
//        TPoint pNew = m_points[i];
//        if (pNew.x > old.x)
//        {
//          p1 = old;
//          p2 = pNew;
//        }
//        else
//        {
//          p1 = pNew;
//          p2 = old;
//        }
//        if ((pNew.x < point.x) == (point.x <= old.x) && (point.y - p1.y) * (p2.x - p1.x)
//              < (p2.y - p1.y) * (point.x - p1.x))
//          isInside = !isInside;

//        old = pNew;
//      }
//      return isInside;
//    }

  private:
    internal_container m_points;
    m2::Rect<coord_type> m_rect;

    STATIC_ASSERT(numeric_limits<coord_type>::is_signed);
  };

  typedef Region<m2::PointF> RegionF;
  typedef Region<m2::PointD> RegionD;
  typedef Region<m2::PointI> RegionI;
}
