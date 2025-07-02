#include "drape_frontend/animation/parabolic_animation.hpp"

#include "drape_frontend/animation/interpolators.hpp"
#include "base/assert.hpp"

#include <algorithm>

namespace df
{

ParabolicAnimation::ParabolicAnimation(ScreenBase const & startScreen,
                                       m2::PointD const & startPos, m2::PointD const & endPos,
                                       double startScale, double endScale,
                                       double startAngle, double endAngle,
                                       double peakScale)
  : Animation(true /* couldBeInterrupted */, false /* couldBeBlended */)
  , m_startPos(startPos)
  , m_endPos(endPos)
  , m_startScale(startScale)
  , m_endScale(endScale)
  , m_peakScale(peakScale)
  , m_startAngle(startAngle)
  , m_endAngle(endAngle)
  , m_elapsedTime(0.0)
{
  m_objects.insert(Animation::Object::MapPlane);
  m_properties.insert(Animation::ObjectProperty::Position);
  m_properties.insert(Animation::ObjectProperty::Scale);
  
  if (std::abs(startAngle - endAngle) > 1e-6)
    m_properties.insert(Animation::ObjectProperty::Angle);

  // Use the standard parabolic animation duration
  m_duration = kParabolicAnimationDuration;
  

}

void ParabolicAnimation::Init(ScreenBase const & screen, TPropertyCache const & properties)
{
  // Animation is already configured in constructor
}

Animation::TObjectProperties const & ParabolicAnimation::GetProperties(Object object) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());
  return m_properties;
}

bool ParabolicAnimation::HasProperty(Object object, ObjectProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void ParabolicAnimation::Advance(double elapsedSeconds)
{
  m_elapsedTime += elapsedSeconds;
}

void ParabolicAnimation::Finish()
{
  m_elapsedTime = m_duration + 1.0;
  Animation::Finish();
}

void ParabolicAnimation::SetMaxDuration(double maxDuration)
{
  if (maxDuration >= 0.0)
    m_duration = std::min(m_duration, maxDuration);
}

void ParabolicAnimation::SetMinDuration(double minDuration)
{
  if (minDuration >= 0.0)
    m_duration = std::max(m_duration, minDuration);
}

double ParabolicAnimation::GetDuration() const
{
  return m_duration;
}

double ParabolicAnimation::GetMaxDuration() const
{
  return Animation::kInvalidAnimationDuration;
}

double ParabolicAnimation::GetMinDuration() const
{
  return Animation::kInvalidAnimationDuration;
}

bool ParabolicAnimation::IsFinished() const
{
  return m_elapsedTime > m_duration;
}

bool ParabolicAnimation::GetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

bool ParabolicAnimation::GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool ParabolicAnimation::GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());

  switch (property)
  {
  case Animation::ObjectProperty::Position:
    value = PropertyValue(targetValue ? m_endPos : GetCurrentPosition());
    return true;
  case Animation::ObjectProperty::Scale:
    {
      double scale = targetValue ? m_endScale : GetCurrentScale();
      value = PropertyValue(scale);
      return true;
    }
  case Animation::ObjectProperty::Angle:
    if (std::abs(m_startAngle - m_endAngle) > 1e-6)
    {
      value = PropertyValue(targetValue ? m_endAngle : GetCurrentAngle());
      return true;
    }
    return false;
  default:
    ASSERT(false, ("Wrong property:", static_cast<int>(property)));
  }

  return false;
}

double ParabolicAnimation::GetT() const
{
  if (IsFinished())
    return 1.0;

  double t = m_elapsedTime / m_duration;
  
  // Apply very gentle quartic ease-out curve for extremely smooth deceleration
  return 1.0 - std::pow(1.0 - t, 4.0);
}

double ParabolicAnimation::GetCurrentScale() const
{
  if (IsFinished())
    return m_endScale;

  // Use the same eased time as position for synchronized movement
  double const t = GetT();
  
  // Create parabolic scale trajectory: zoom out to peak, then zoom back in
  // At t=0: startScale, at t=0.5: peakScale, at t=1.0: endScale
  double scale;
  if (t <= 0.5)
  {
    // First half: zoom out to peak
    double const t1 = t * 2.0; // 0 to 1
    scale = m_startScale + (m_peakScale - m_startScale) * t1;
  }
  else
  {
    // Second half: zoom back in
    double const t2 = (t - 0.5) * 2.0; // 0 to 1
    scale = m_peakScale + (m_endScale - m_peakScale) * t2;
  }
  
  return scale;
}

m2::PointD ParabolicAnimation::GetCurrentPosition() const
{
  double const t = GetT();
  // Linear interpolation for position (the parabolic effect comes from scale changes)
  return m_startPos + (m_endPos - m_startPos) * t;
}

double ParabolicAnimation::GetCurrentAngle() const
{
  double const t = GetT();
  return m_startAngle + (m_endAngle - m_startAngle) * t;
}

} // namespace df 
