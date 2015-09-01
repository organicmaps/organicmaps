#include "kinetic_scroller.hpp"
#include "visual_params.hpp"
#include "base/logging.hpp"

namespace df
{

double const kKineticDuration = 0.375;
double const kKineticFeedback = 0.2;
double const kKineticFadeoff = 5.0;
double const kKineticThreshold = 10.0;
double const kKineticInertia = 0.8;

class KineticScrollAnimation : public BaseModelViewAnimation
{
public:
  // startRect - mercator visible on screen rect in moment when user release fingers
  // direction - mercator space direction of moving. length(direction) - mercator distance on wich map will be offset
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
    // current position = target position - amplutide * e ^ (elapsed / duration)
    // we calculate current position not based on start position, but based on target position
    return m2::AnyRectD(m_targetCenter - m_direction * exp(-kKineticFadeoff * GetT()), m_angle, m_localRect);
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

bool KineticScroller::IsActive() const
{
  return m_lastTimestamp > 0.0;
}

void KineticScroller::GrabViewRect(ScreenBase const & modelView, double timeStamp)
{
  // In KineitcScroller we store m_direction in mixed state
  // Direction in mercator space, and length(m_direction) in pixel space
  // We need same reaction on different zoom levels, and should calculate velocity on pixel space
  ASSERT_GREATER(m_lastTimestamp, 0.0, ());
  ASSERT_GREATER(timeStamp, m_lastTimestamp, ());
  double elapsed = timeStamp - m_lastTimestamp;

  m2::PointD lastCenter = m_lastRect.GlobalCenter();
  m2::PointD currentCenter = modelView.GlobalRect().GlobalCenter();
  double pxDeltaLength = (modelView.GtoP(currentCenter) - modelView.GtoP(lastCenter)).Length();
  m2::PointD delta = (currentCenter - lastCenter);
  if (!delta.IsAlmostZero())
    delta = delta.Normalize();

  // velocity on pixels
  double v = pxDeltaLength / elapsed;
  // at this point length(m_direction) already in pixel space, and delta normalized
  m_direction = delta * kKineticInertia * v + m_direction * (1.0 - kKineticInertia);

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
  static double kVelocityThreshold = kKineticThreshold * VisualParams::Instance().GetVisualScale();
  if (m_direction.Length() < kVelocityThreshold)
    return unique_ptr<BaseModelViewAnimation>();

  // Before we start animation we have to convert length(m_direction) from pixel space to mercator space
  m2::PointD center = m_lastRect.GlobalCenter();
  double glbLength = kKineticFeedback * (modelView.PtoG(modelView.GtoP(center) + m_direction) - center).Length();
  return unique_ptr<BaseModelViewAnimation>(new KineticScrollAnimation(m_lastRect,
                                                                       m_direction.Normalize() * glbLength,
                                                                       kKineticDuration));
}

} // namespace df
