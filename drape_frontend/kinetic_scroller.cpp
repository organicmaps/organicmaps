#include "kinetic_scroller.hpp"
#include "visual_params.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace df
{

double const kKineticDuration = 1.5;
double const kKineticFadeoff = 4.0;
double const kKineticThreshold = 50.0;
double const kKineticAcceleration = 0.4;
double const kKineticMaxSpeedStart = 1000.0; // pixels per second
double const kKineticMaxSpeedEnd = 10000.0; // pixels per second

double CalculateKineticMaxSpeed(ScreenBase const & modelView)
{
  double const kMinZoom = 1.0;
  double const kMaxZoom = scales::UPPER_STYLE_SCALE + 1.0;
  double const zoomLevel = my::clamp(fabs(log(modelView.GetScale()) / log(2.0)), kMinZoom, kMaxZoom);
  double const lerpCoef = 1.0 - ((zoomLevel - kMinZoom) / (kMaxZoom - kMinZoom));

  return (kKineticMaxSpeedStart * lerpCoef + kKineticMaxSpeedEnd * (1.0 - lerpCoef)) * VisualParams::Instance().GetVisualScale();
}

class KineticScrollAnimation : public Animation
{
public:
  // startRect - mercator visible on screen rect in moment when user release fingers.
  // direction - mercator space direction of moving. length(direction) - mercator distance on wich map will be offset.
  KineticScrollAnimation(m2::PointD const & startPos, m2::PointD const & direction, double duration)
    : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
    , m_endPos(startPos + direction)
    , m_direction(direction)
    , m_duration(duration)
    , m_elapsedTime(0.0)
  {
    SetInterruptedOnCombine(true);
    m_objects.insert(Animation::MapPlane);
    m_properties.insert(Animation::Position);
  }

  Animation::Type GetType() const override { return Animation::KineticScroll; }

  TAnimObjects const & GetObjects() const override
  {
    return m_objects;
  }

  bool HasObject(TObject object) const override
  {
    return m_objects.find(object) != m_objects.end();
  }

  TObjectProperties const & GetProperties(TObject object) const override
  {
    ASSERT(HasObject(object), ());
    return m_properties;
  }

  bool HasProperty(TObject object, TProperty property) const override
  {
    return HasObject(object) && m_properties.find(property) != m_properties.end();
  }

  void SetMaxDuration(double maxDuration) override
  {
    if (m_duration > maxDuration)
      m_duration = maxDuration;
  }

  double GetDuration() const override { return m_duration; }
  bool IsFinished() const override { return m_elapsedTime >= m_duration; }

  void Advance(double elapsedSeconds) override
  {
    m_elapsedTime += elapsedSeconds;
  }

  void Finish() override
  {
    m_elapsedTime = m_duration;
    Animation::Finish();
  }

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override
  {
    ASSERT(HasProperty(object, property), ());
    // Current position = target position - amplutide * e ^ (elapsed / duration).
    // We calculate current position not based on start position, but based on target position.
    value = PropertyValue(m_endPos - m_direction * exp(-kKineticFadeoff * GetT()));
    return true;
  }

private:
  double GetT() const
  {
    return IsFinished() ? 1.0 : m_elapsedTime / m_duration;
  }

  m2::PointD m_endPos;
  m2::PointD m_direction;
  double m_duration;
  double m_elapsedTime;
  TAnimObjects m_objects;
  TObjectProperties m_properties;
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
  // In KineitcScroller we store m_direction in mixed state.
  // Direction in mercator space, and length(m_direction) in pixel space.
  // We need same reaction on different zoom levels, and should calculate velocity on pixel space.
  ASSERT_GREATER(m_lastTimestamp, 0.0, ());
  ASSERT_GREATER(timeStamp, m_lastTimestamp, ());
  double elapsed = timeStamp - m_lastTimestamp;

  m2::PointD lastCenter = m_lastRect.GlobalCenter();
  m2::PointD currentCenter = modelView.GlobalRect().GlobalCenter();
  double pxDeltaLength = (modelView.GtoP(currentCenter) - modelView.GtoP(lastCenter)).Length();
  m2::PointD delta = (currentCenter - lastCenter);
  if (!delta.IsAlmostZero())
  {
    delta = delta.Normalize();

    // Velocity on pixels.
    double const v = min(pxDeltaLength / elapsed, CalculateKineticMaxSpeed(modelView));

    // At this point length(m_direction) already in pixel space, and delta normalized.
    m_direction = delta * v;
  }
  else
  {
    m_direction = m2::PointD::Zero();
  }

  m_lastTimestamp = timeStamp;
  m_lastRect = modelView.GlobalRect();
}

void KineticScroller::CancelGrab()
{
  m_lastTimestamp = -1;
  m_direction = m2::PointD::Zero();
}

drape_ptr<Animation> KineticScroller::CreateKineticAnimation(ScreenBase const & modelView)
{
  static double kVelocityThreshold = kKineticThreshold * VisualParams::Instance().GetVisualScale();
  if (m_direction.Length() < kVelocityThreshold)
    return drape_ptr<Animation>();

  // Before we start animation we have to convert length(m_direction) from pixel space to mercator space.
  m2::PointD center = m_lastRect.GlobalCenter();
  double const offset = (modelView.PtoG(modelView.GtoP(center) + m_direction) - center).Length();
  double const glbLength = kKineticAcceleration * offset;
  m2::PointD const glbDirection = m_direction.Normalize() * glbLength;
  m2::PointD const targetCenter = center + glbDirection;
  if (!df::GetWorldRect().IsPointInside(targetCenter))
    return drape_ptr<Animation>();

  return make_unique_dp<KineticScrollAnimation>(center, glbDirection, kKineticDuration);
}

} // namespace df
