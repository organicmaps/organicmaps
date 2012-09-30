#include "anyrect_interpolation.hpp"

namespace anim
{
  AnyRectInterpolation::AnyRectInterpolation(m2::AnyRectD const & startRect,
                                             m2::AnyRectD const & endRect,
                                             double rotationSpeed,
                                             m2::AnyRectD & outRect)
    : m_interval(rotationSpeed * fabs(ang::GetShortestDistance(startRect.Angle().val(),
                                                               endRect.Angle().val())) / (2 * math::pi)),
      m_angleInt(startRect.Angle().val(),
                 endRect.Angle().val(),
                 rotationSpeed,
                 m_curAngle),
      m_segmentInt(startRect.GlobalCenter(),
                   endRect.GlobalCenter(),
                   m_interval,
                   m_curCenter),
      m_sizeXInt(startRect.GetLocalRect().SizeX(),
                 endRect.GetLocalRect().SizeX(),
                 m_interval,
                 m_curSizeX),
      m_sizeYInt(startRect.GetLocalRect().SizeY(),
                 endRect.GetLocalRect().SizeY(),
                 m_interval,
                 m_curSizeY),
      m_startRect(startRect),
      m_endRect(endRect),
      m_outRect(outRect)
  {
  }

  void AnyRectInterpolation::OnStart(double ts)
  {
    m_startTime = ts;

    m_angleInt.OnStart(ts);
    m_segmentInt.OnStart(ts);
    m_sizeXInt.OnStart(ts);
    m_sizeYInt.OnStart(ts);

    m_outRect = m_startRect;

    anim::Task::OnStart(ts);
  }

  void AnyRectInterpolation::OnStep(double ts)
  {
    if (ts - m_startTime >= m_interval)
    {
      End();
      return;
    }

    if (!IsRunning())
      return;

    m_angleInt.OnStep(ts);
    m_segmentInt.OnStep(ts);
    m_sizeXInt.OnStep(ts);
    m_sizeYInt.OnStep(ts);

    m_outRect = m2::AnyRectD(m_curCenter, m_curAngle, m2::RectD(-m_curSizeX / 2, -m_curSizeY / 2, m_curSizeX / 2, m_curSizeY / 2));

    anim::Task::OnStep(ts);
  }

  void AnyRectInterpolation::OnEnd(double ts)
  {
    m_angleInt.OnEnd(ts);
    m_segmentInt.OnEnd(ts);
    m_sizeXInt.OnEnd(ts);
    m_sizeYInt.OnEnd(ts);

    m_outRect = m_endRect;

    anim::Task::OnEnd(ts);
  }
}
