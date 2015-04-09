#pragma once

#include "geometry/point2d.hpp"

namespace graphics
{
  template <typename T>
  class Path
  {
  private:

    vector<m2::Point<T> > m_pts;

  public:

    void reset(m2::Point<T> const & pt)
    {
      m_pts.clear();
      m_pts.push_back(pt);
    }

    void lineRel(m2::Point<T> const & pt)
    {
      ASSERT(!m_pts.empty(), ());
      m2::Point<T> const & p = m_pts.back();
      m_pts.push_back(p + pt);
    }

    void eclipseArcRel(m2::Point<T> const & pt)
    {
      /// TODO : write implementation
    }

    m2::Point<T> const * points() const
    {
      ASSERT(!m_pts.empty(), ());
      return &m_pts[0];
    }

    unsigned size() const
    {
      return m_pts.size();
    }
  };
}
