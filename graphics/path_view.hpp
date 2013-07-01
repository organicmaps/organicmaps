#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/angles.hpp"

namespace graphics
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

  class PathView
  {
  private:

    m2::PointD const * m_pts;
    size_t m_ptsCount;
    bool m_isReverse;

  public:

    PathView();
    PathView(m2::PointD const * pts, size_t ptsCount);

    size_t size() const;

    m2::PointD const & get(size_t i) const;

    void setIsReverse(bool flag);
    bool isReverse() const;

    PathPoint const offsetPoint(PathPoint const & pp, double offset) const;
    PivotPoint findPivotPoint(PathPoint const & pp, double advance) const;

    PathPoint const front() const;
  };

}
