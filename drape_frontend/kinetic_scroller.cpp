#include "kinetic_scroller.hpp"
#include "visual_params.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
double const kKineticDuration = 1.5;
double const kKineticFadeoff = 4.0;
double const kKineticThreshold = 50.0;
double const kKineticAcceleration = 0.4;
double const kKineticMaxSpeedStart = 1000.0; // pixels per second
double const kKineticMaxSpeedEnd = 10000.0; // pixels per second
double const kInstantVelocityThresholdUnscaled = 200.0; // pixels per second

double CalculateKineticMaxSpeed(ScreenBase const & modelView)
{
  double const lerpCoef = 1.0 - GetNormalizedZoomLevel(modelView.GetScale());
  return (kKineticMaxSpeedStart * lerpCoef + kKineticMaxSpeedEnd * (1.0 - lerpCoef)) *
         VisualParams::Instance().GetVisualScale();
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
    m_objects.insert(Animation::Object::MapPlane);
    m_properties.insert(Animation::ObjectProperty::Position);
  }

  Animation::Type GetType() const override { return Animation::Type::KineticScroll; }

  TAnimObjects const & GetObjects() const override
  {
    return m_objects;
  }

  bool HasObject(Object object) const override
  {
    return m_objects.find(object) != m_objects.end();
  }

  TObjectProperties const & GetProperties(Object object) const override
  {
    ASSERT(HasObject(object), ());
    return m_properties;
  }

  bool HasProperty(Object object, ObjectProperty property) const override
  {
    return HasObject(object) && m_properties.find(property) != m_properties.end();
  }

  void SetMaxDuration(double maxDuration) override
  {
    if (m_duration > maxDuration)
      m_duration = maxDuration;
  }

  void SetMinDuration(double minDuration) override {}
  double GetMaxDuration() const override { return Animation::kInvalidAnimationDuration; }
  double GetMinDuration() const override { return Animation::kInvalidAnimationDuration; }

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

  bool GetProperty(Object object, ObjectProperty property, PropertyValue & value) const override
  {
    ASSERT(HasProperty(object, property), ());
    // Current position = target position - amplutide * e ^ (elapsed / duration).
    // We calculate current position not based on start position, but based on target position.
    value = PropertyValue(m_endPos - m_direction * exp(-kKineticFadeoff * GetT()));
    return true;
  }

  bool GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const override
  {
    ASSERT(HasProperty(object, property), ());
    value = PropertyValue(m_endPos);
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

void KineticScroller::Init(ScreenBase const & modelView)
{
  ASSERT(!m_isActive, ());
  m_isActive = true;
  m_lastRect = modelView.GlobalRect();
  m_lastTimestamp = std::chrono::steady_clock::now();
  m_updatePosition = modelView.GlobalRect().GlobalCenter();
  m_updateTimestamp = m_lastTimestamp;
}

void KineticScroller::Update(ScreenBase const & modelView)
{
  ASSERT(m_isActive, ());
  using namespace std::chrono;
  auto const nowTimestamp = std::chrono::steady_clock::now();
  auto const curPos = modelView.GlobalRect().GlobalCenter();

  double const instantPixelLen = (modelView.GtoP(curPos) - modelView.GtoP(m_updatePosition)).Length();
  auto const updateElapsed = duration_cast<duration<double>>(nowTimestamp - m_updateTimestamp).count();
  m_instantVelocity = (updateElapsed >= 1e-5) ? instantPixelLen / updateElapsed : 0.0;

  m_updateTimestamp = nowTimestamp;
  m_updatePosition = curPos;
}

bool KineticScroller::IsActive() const
{
  return m_isActive;
}

m2::PointD KineticScroller::GetDirection(ScreenBase const & modelView) const
{
  // In KineticScroller we store m_direction in mixed state.
  // Direction in mercator space, and length(m_direction) in pixel space.
  // We need same reaction on different zoom levels, and should calculate velocity on pixel space.
  ASSERT(m_isActive, ());
  using namespace std::chrono;
  auto const nowTimestamp = steady_clock::now();
  auto const elapsed = duration_cast<duration<double>>(nowTimestamp - m_lastTimestamp).count();
  m2::PointD const currentCenter = modelView.GlobalRect().GlobalCenter();

  m2::PointD const lastCenter = m_lastRect.GlobalCenter();
  double const pxDeltaLength = (modelView.GtoP(currentCenter) - modelView.GtoP(lastCenter)).Length();
  m2::PointD delta = currentCenter - lastCenter;
  if (!delta.IsAlmostZero())
  {
    delta = delta.Normalize();

    // Velocity on pixels.
    double const v = std::min(pxDeltaLength / elapsed, CalculateKineticMaxSpeed(modelView));

    // At this point length(m_direction) already in pixel space, and delta normalized.
    return delta * v;
  }
  return m2::PointD::Zero();
}

void KineticScroller::Cancel()
{
  m_isActive = false;
}

drape_ptr<Animation> KineticScroller::CreateKineticAnimation(ScreenBase const & modelView)
{
  static double vs = VisualParams::Instance().GetVisualScale();
  static double kVelocityThreshold = kKineticThreshold * vs;
  static double kInstantVelocityThreshold = kInstantVelocityThresholdUnscaled * vs;

  if (m_instantVelocity < kInstantVelocityThreshold)
  {
    Cancel();
    return drape_ptr<Animation>();
  }

  auto const direction = GetDirection(modelView);
  Cancel();

  if (direction.Length() < kVelocityThreshold)
    return drape_ptr<Animation>();

  // Before we start animation we have to convert length(m_direction) from pixel space to mercator space.
  m2::PointD const center = modelView.GlobalRect().GlobalCenter();
  double const offset = (modelView.PtoG(modelView.GtoP(center) + direction) - center).Length();
  double const glbLength = kKineticAcceleration * offset;
  m2::PointD const glbDirection = direction.Normalize() * glbLength;
  m2::PointD const targetCenter = center + glbDirection;
  if (!df::GetWorldRect().IsPointInside(targetCenter))
    return drape_ptr<Animation>();

  return make_unique_dp<KineticScrollAnimation>(center, glbDirection, kKineticDuration);
}
}  // namespace df
