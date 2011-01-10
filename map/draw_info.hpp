#pragma once

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"
#include "../std/algorithm.hpp"


namespace di
{
  class PathInfo
  {
    mutable double m_length;
    double m_offset;

  public:
    vector<m2::PointD> m_path;

    // -1.0 means "not" initialized
    PathInfo(double offset = -1.0) : m_length(-1.0), m_offset(offset) {}

    void swap(PathInfo & r)
    {
      m_path.swap(r.m_path);
      std::swap(m_length, r.m_length);
      std::swap(m_offset, r.m_offset);
    }

    void push_back(m2::PointD const & p)
    {
      m_path.push_back(p);
    }

    size_t size() const { return m_path.size(); }

    void SetLength(double len) { m_length = len; }

    double GetLength() const
    {
      // m_length not initialized - calculate it
      //if (m_length < 0.0)
      //{
      //  m_length = 0.0;
      //  for (size_t i = 1; i < m_path.size(); ++i)
      //    m_length += m_path[i-1].Length(m_path[i]);
      //}

      ASSERT ( m_length > 0.0, (m_length) );
      return m_length;
    }

    double GetOffset() const
    {
      ASSERT ( m_offset >= 0.0, (m_offset) );
      return m_offset;
    }
  };

  class AreaInfo
  {
    mutable m2::PointD m_center;

    inline static double ic() { return -1.0E10; }

  public:
    vector<m2::PointD> m_path;

    AreaInfo() : m_center(ic(), ic()) {}

    void reserve(size_t ptsCount)
    {
      m_path.reserve(ptsCount);
    }

    void swap(AreaInfo & r)
    {
      m_path.swap(r.m_path);
      std::swap(m_center, r.m_center);
    }

    void push_back(m2::PointD const & pt)
    {
      m_path.push_back(pt);
    }

    size_t size() const { return m_path.size(); }

    // lazy evaluation of area's center point
    m2::PointD GetCenter() const
    {
      if (m_center.EqualDxDy(m2::PointD(ic(), ic()), 1.0E-8))
      {
        m_center = m2::PointD(0.0, 0.0);
        size_t count = m_path.size();
        for (size_t i = 0; i < count; ++i)
          m_center += m_path[i];

        m_center *= (1.0/static_cast<double>(count));
      }

      return m_center;
    }
  };
}

inline void swap(di::PathInfo & p1, di::PathInfo & p2)
{
  p1.swap(p2);
}

inline void swap(di::AreaInfo & p1, di::AreaInfo & p2)
{
  p1.swap(p2);
}
