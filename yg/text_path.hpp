#pragma once

#include "../geometry/point2d.hpp"
#include "glyph_cache.hpp"

namespace yg
{
  struct PathPoint
  {
    int m_i;
    m2::PointD m_pt;
    PathPoint(int i = -1,
              m2::PointD const & pt = m2::PointD());
  };

  struct PivotPoint
  {
    double m_angle;
    PathPoint m_pp;
    PivotPoint(double angle = 0, PathPoint const & pp = PathPoint());
  };

  class TextPath
  {
    m2::PointD const * m_arr;
    size_t m_size;
    bool m_reverse;
  public:
    TextPath(m2::PointD const * arr, size_t sz, double fullLength, double & pathOffset);

    size_t size() const;

    m2::PointD get(size_t i) const;
    m2::PointD operator[](size_t i) const;

    PathPoint const offsetPoint(PathPoint const & pp, double offset);

    PivotPoint findPivotPoint(PathPoint const & pp, GlyphMetrics const & sym, double kern);
  };
}
