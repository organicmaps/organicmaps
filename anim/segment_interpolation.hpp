#pragma once

#include "task.hpp"
#include "../geometry/point2d.hpp"

namespace anim
{
  class SegmentInterpolation : public Task
  {
  private:

    m2::PointD m_startPt;
    m2::PointD m_endPt;
    m2::PointD & m_outPt;
    double m_startTime;
    double m_interval;

  public:

    SegmentInterpolation(m2::PointD const & startPt,
                         m2::PointD const & endPt,
                         double interval,
                         m2::PointD & outPt);

    void Reset(m2::PointD const & start, m2::PointD const & end, double interval);

    void OnStart(double ts);
    void OnStep(double ts);
    void OnEnd(double ts);
  };
}

