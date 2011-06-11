#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/angles.hpp"
#include "glyph_cache.hpp"

namespace yg
{
  struct PathPoint
  {
    int m_i; //< segment number
    ang::AngleD m_segAngle;
    m2::PointD m_pt; //< point on segment
    PathPoint(int i = -1,
              ang::AngleD const & segAngle = ang::AngleD(),
              m2::PointD const & pt = m2::PointD());
  };

  struct PivotPoint
  {
    ang::AngleD m_angle;
    PathPoint m_pp;
    PivotPoint(ang::AngleD const & angle = ang::AngleD(), PathPoint const & pp = PathPoint());
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
