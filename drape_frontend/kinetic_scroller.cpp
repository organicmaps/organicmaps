#include "kinetic_scroller.hpp"
#include "visual_params.hpp"
#include "base/logging.hpp"

namespace df
{

class KineticScrollAnimation : public BaseModelViewAnimation
{
public:
  KineticScrollAnimation(m2::AnyRectD const & startRect, m2::PointD const & direction, double duration)
    : BaseModelViewAnimation(duration)
    , m_targetCenter(startRect.GlobalCenter() + direction)
    , m_angle(startRect.Angle())
    , m_localRect(startRect.GetLocalRect())
    , m_direction(direction)
  {
  }

  m2::AnyRectD GetCurrentRect() const override
  {
    m2::PointD center = -m_direction * exp(-GetT());
    m2::AnyRectD rect(m_targetCenter + center, m_angle, m_localRect);
    return rect;
  }

  m2::AnyRectD GetTargetRect() const override
  {
    return GetCurrentRect();
  }

private:
  m2::PointD m_targetCenter;
  ang::AngleD m_angle;
  m2::RectD m_localRect;
  m2::PointD m_direction;
};

KineticScroller::KineticScroller()
  : m_lastTimestamp(-1.0)
  , m_direction(m2::PointD::Zero())
{
}

void KineticScroller::InitGrab(ScreenBase const & modelView, double timeStamp)
{
  ASSERT_LESS(m_lastTimestamp, 0.0, ());
  m_lastTimestamp = timeStamp;
  m_lastRect = modelView.GlobalRect();
}

void KineticScroller::GrabViewRect(ScreenBase const & modelView, double timeStamp)
{
  ASSERT_GREATER(m_lastTimestamp, 0.0, ());
  ASSERT_GREATER(timeStamp, m_lastTimestamp, ());
  double elapsed = timeStamp - m_lastTimestamp;

  m2::PointD lastCenter = m_lastRect.GlobalCenter();
  m2::PointD currentCenter = modelView.GlobalRect().GlobalCenter();
  double pxDeltaLength = (modelView.GtoP(currentCenter) - modelView.GtoP(lastCenter)).Length();
  m2::PointD delta = (currentCenter - lastCenter);
  if (!delta.IsAlmostZero())
    delta = delta.Normalize();

  double v = pxDeltaLength / elapsed;
  m_direction = delta * 0.8 * v + m_direction * 0.2;

  m_lastTimestamp = timeStamp;
  m_lastRect = modelView.GlobalRect();
}

void KineticScroller::CancelGrab()
{
  m_lastTimestamp = -1;
  m_direction = m2::PointD::Zero();
}

unique_ptr<BaseModelViewAnimation> KineticScroller::CreateKineticAnimation(ScreenBase const & modelView)
{
  static double VELOCITY_THRESHOLD = 10.0 * VisualParams::Instance().GetVisualScale();
  if (m_direction.Length() < VELOCITY_THRESHOLD)
    return unique_ptr<BaseModelViewAnimation>();

  double const KINETIC_DURATION = 0.375;
  m2::PointD center = m_lastRect.GlobalCenter();
  double glbLength = 0.5 * (modelView.PtoG(modelView.GtoP(center) + m_direction) - center).Length();
  return unique_ptr<BaseModelViewAnimation>(new KineticScrollAnimation(m_lastRect,
                                                                       m_direction.Normalize() * glbLength,
                                                                       KINETIC_DURATION));
}

} // namespace df
