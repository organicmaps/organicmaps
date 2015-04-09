#include "anim/segment_interpolation.hpp"
#include "anim/controller.hpp"


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
    m_speed = (m_endPt - m_startPt) / m_interval;
  }

  void SegmentInterpolation::Reset(m2::PointD const & start, m2::PointD const & end, double interval)
  {
    ASSERT_GREATER(interval, 0.0, ());

    m_startPt = start;
    m_endPt = end;
    m_outPt = m_startPt;
    m_interval = interval;

    m_speed = (m_endPt - m_startPt) / m_interval;
    m_startTime = GetController()->GetCurrentTime();
    SetState(EReady);
  }

  void SegmentInterpolation::OnStart(double ts)
  {
    m_startTime = ts;
    m_outPt = m_startPt;
    Task::OnStart(ts);
  }

  void SegmentInterpolation::OnStep(double ts)
  {
    double const elapsed = ts - m_startTime;
    if (elapsed >= m_interval)
    {
      End();
      return;
    }

    if (!IsRunning())
      return;

    m_outPt = m_startPt + m_speed * elapsed;

    Task::OnStep(ts);
  }

  void SegmentInterpolation::OnEnd(double ts)
  {
    // ensuring that the final value was reached
    m_outPt = m_endPt;
    Task::OnEnd(ts);
  }

  SafeSegmentInterpolation::SafeSegmentInterpolation(m2::PointD const & startPt, m2::PointD const & endPt,
                                                     double interval)
    : TBase(startPt, endPt, interval, m_pt)
  {
    m_pt = startPt;
  }

  void SafeSegmentInterpolation::ResetDestParams(m2::PointD const & dstPt, double interval)
  {
    Reset(GetCurrentValue(), dstPt, interval);
  }

  m2::PointD const & SafeSegmentInterpolation::GetCurrentValue() const
  {
    return m_pt;
  }
}
