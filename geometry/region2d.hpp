#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"

#include "../std/vector.hpp"

namespace m2
{
  template <class TPoint>
  class Region
  {
  public:
    typedef TPoint value_type;
    typedef typename TPoint::value_type coord_type;

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

    void AddPoint(TPoint const & pt)
    {
      m_points.push_back(pt);
      m_rect.Add(pt);
    }

    m2::Rect<coord_type> Rect() const { return m_rect; }

    bool IsValid() const { return m_points.size() > 2; }

    bool Contains(TPoint const & pt) const
    {
      // quick and dirty intersect
      if (m_points.size() < 3 || !m_rect.IsPointInside(pt))
        return false;

      bool isInside = false;
      size_t const size = m_points.size();
      for (size_t i = 0; i < size; ++i)
      {
        isInside = false;
        size_t i1 = i < size - 1 ? i + 1 : 0;
        while (!isInside)
        {
         size_t i2 = i1 + 1;
         if (i2 >= size)
           i2 = 0;
         if (i2 == (i < size - 1 ? i + 1 : 0))
           break;
         size_t S = abs(m_points[i1].x * (m_points[i2].y - m_points[i].y) +
            m_points[i2].x * (m_points[i].y - m_points[i1].y) +
            m_points[i].x  * (m_points[i1].y - m_points[i2].y));
         size_t S1 = abs(m_points[i1].x * (m_points[i2].y - pt.y) +
            m_points[i2].x * (pt.y - m_points[i1].y) +
            pt.x * (m_points[i1].y - m_points[i2].y));
         size_t S2 = abs(m_points[i].x * (m_points[i2].y - pt.y) +
            m_points[i2].x * (pt.y - m_points[i].y) +
            pt.x * (m_points[i].y - m_points[i2].y));
         size_t S3 = abs(m_points[i1].x * (m_points[i].y - pt.y) +
            m_points[i].x * (pt.y - m_points[i1].y) +
            pt.x * (m_points[i1].y - m_points[i].y));
         if (S == S1 + S2 + S3)
         {
          isInside = true;
          break;
         }
         i1 = i1 + 1;
         if (i1 >= size)
           i1 = 0;
        }
        if (!isInside)
          break;
       }
       return isInside;
    }
/*
    bool Contains(TPoint const & pt) const
    {
      // Raycast point in polygon method
      size_t const size = m_points.size();
      bool inPoly = false;
      size_t i;
      size_t j = size - 1;

      for (i = 0; i < size; ++i)
      {
        TPoint vertex1 = m_points[i];
        TPoint vertex2 = m_points[j];

        if ((vertex1.x < pt.x == vertex2.x >= pt.x) || vertex2.x < pt.x && vertex1.x >= pt.x)
        {
          if (vertex1.y + (pt.x - vertex1.x) / (vertex2.x - vertex1.x)
              * (vertex2.y - vertex1.y) < pt.y)
          {
            inPoly = !inPoly;
          }
        }

        j = i;
      }

      return inPoly;
    }

    bool Contains(TPoint const & pt) const
    {
      size_t const size = m_points.size();
      bool isInside = false;
      for (size_t i = 0, j = size - 1; i < size; j = i++)
      {
        if ( ((m_points[i].y > pt.y) != (m_points[j].y > pt.y)) &&
            (pt.x < (m_points[j].x - m_points[i].x)
             * (pt.y - m_points[i].y) / (m_points[j].y - m_points[i].y) + m_points[i].x) )
          isInside = !isInside;
      }
      return isInside;
    }

    bool Contains(TPoint const & point) const
    {
      size_t const size = m_points.size();
      if (size < 3)
        return false;

      bool isInside = false;
      TPoint old = m_points[size - 1];
      TPoint p1, p2;
      for (size_t i = 0; i < size - 1; ++i)
      {
        TPoint pNew = m_points[i];
        if (pNew.x > old.x)
        {
          p1 = old;
          p2 = pNew;
        }
        else
        {
          p1 = pNew;
          p2 = old;
        }
        if ((pNew.x < point.x) == (point.x <= old.x) && (point.y - p1.y) * (p2.x - p1.x)
              < (p2.y - p1.y) * (point.x - p1.x))
          isInside = !isInside;

        old = pNew;
      }
      return isInside;
    }
    */

  private:
    vector<TPoint> m_points;
    m2::Rect<coord_type> m_rect;
  };

  typedef Region<m2::PointF> RegionF;
  typedef Region<m2::PointD> RegionD;
  typedef Region<m2::PointU> RegionU;
  typedef Region<m2::PointI> RegionI;

}
