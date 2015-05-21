#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/vector.hpp"
#include "std/algorithm.hpp"

namespace di
{
  class PathInfo
  {
    double m_fullL;
    double m_offset;

  public:
    vector<m2::PointD> m_path;

    // -1.0 means "not" initialized
    PathInfo(double offset = -1.0) : m_fullL(-1.0), m_offset(offset) {}

    void swap(PathInfo & r)
    {
      m_path.swap(r.m_path);
      std::swap(m_fullL, r.m_fullL);
      std::swap(m_offset, r.m_offset);
    }

    void push_back(m2::PointD const & p)
    {
      m_path.push_back(p);
    }

    size_t size() const { return m_path.size(); }

    void SetFullLength(double len) { m_fullL = len; }
    double GetFullLength() const
    {
      ASSERT ( m_fullL > 0.0, (m_fullL) );
      return m_fullL;
    }

    double GetLength() const
    {
      double sum = 0.0;
      for (size_t i = 1; i < m_path.size(); ++i)
      {
        double const l = m_path[i-1].Length(m_path[i]);
        sum += l;
      }
      return sum;
    }

    double GetOffset() const
    {
      ASSERT ( m_offset >= 0.0, (m_offset) );
      return m_offset;
    }

    bool GetSmPoint(double part, m2::PointD & pt) const
    {
      double sum = -GetFullLength() * part + m_offset;
      if (sum > 0.0) return false;

      for (size_t i = 1; i < m_path.size(); ++i)
      {
        double const l = m_path[i-1].Length(m_path[i]);
        sum += l;
        if (sum >= 0.0)
        {
          double const factor = (l - sum) / l;
          ASSERT_GREATER_OR_EQUAL ( factor, 0.0, () );

          pt.x = factor * (m_path[i].x - m_path[i-1].x) + m_path[i-1].x;
          pt.y = factor * (m_path[i].y - m_path[i-1].y) + m_path[i-1].y;
          return true;
        }
      }

      return false;
    }

    m2::RectD GetLimitRect() const
    {
      m2::RectD rect;
      for (size_t i = 0; i < m_path.size(); ++i)
        rect.Add(m_path[i]);
      return rect;
    }
  };
}

inline void swap(di::PathInfo & p1, di::PathInfo & p2)
{
  p1.swap(p2);
}
