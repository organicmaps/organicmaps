#pragma once

#include "anim/task.hpp"

#include "geometry/point2d.hpp"


namespace anim
{
  class SegmentInterpolation : public Task
  {
    m2::PointD m_startPt;
    m2::PointD m_endPt;
    m2::PointD & m_outPt;
    double m_interval;

    m2::PointD m_speed;
    double m_startTime;

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

  class SafeSegmentInterpolation : public SegmentInterpolation
  {
    typedef SegmentInterpolation TBase;

  public:
    SafeSegmentInterpolation(m2::PointD const & startPt,
                             m2::PointD const & endPt,
                             double interval);

    void ResetDestParams(m2::PointD const & dstPt, double interval);
    m2::PointD const & GetCurrentValue() const;

  private:
    m2::PointD m_pt;
  };
}
