#include "segment_interpolation.hpp"

namespace anim
{
  SegmentInterpolation::SegmentInterpolation(m2::PointD const & startPt,
                                             m2::PointD const & endPt,
                                             double interval,
                                             m2::PointD & outPt)
    : m_startPt(startPt),
      m_endPt(endPt),
      m_outPt(outPt),
      m_interval(interval)
  {
    m_deltaPt = m_endPt - m_startPt;
  }

  void SegmentInterpolation::OnStart(double ts)
  {
    m_startTime = ts;
    m_outPt = m_startPt;
    Task::OnStart(ts);
  }

  void SegmentInterpolation::OnStep(double ts)
  {
    if (ts - m_startTime >= m_interval)
    {
      End();
      return;
    }

    if (!IsRunning())
      return;

    double elapsedSec = ts - m_startTime;

    m_outPt = m_startPt + m_deltaPt * (elapsedSec / m_interval);

    Task::OnStep(ts);
  }

  void SegmentInterpolation::OnEnd(double ts)
  {
    // ensuring that the final value was reached
    m_outPt = m_endPt;
    Task::OnEnd(ts);
  }
}
