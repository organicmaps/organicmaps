#include "drape_frontend/animation/linear_animation.hpp"

#include "base/assert.hpp"

#include <algorithm>

namespace df
{

MapLinearAnimation::MapLinearAnimation(m2::PointD const & startPos, m2::PointD const & endPos, double startAngle,
                                       double endAngle, double startScale, double endScale,
                                       ScreenBase const & convertor)
  : Animation(true /* couldBeInterrupted */, false /* couldBeBlended */)
  , m_angleInterpolator(startAngle, endAngle)
  , m_positionInterpolator(startPos, endPos, convertor)
  , m_scaleInterpolator(startScale, endScale, false /* isAutoZoom */)
{
  m_objects.insert(Animation::Object::MapPlane);

  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Position);

  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Angle);

  if (m_scaleInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Scale);
}

MapLinearAnimation::MapLinearAnimation() : Animation(true /* couldBeInterrupted */, false /* couldBeBlended */)
{
  m_objects.insert(Animation::Object::MapPlane);
}

void MapLinearAnimation::Init(ScreenBase const & screen, TPropertyCache const & properties)
{
  ScreenBase currentScreen;
  GetCurrentScreen(properties, screen, currentScreen);

  double minDuration = m_positionInterpolator.GetMinDuration();
  double maxDuration = m_positionInterpolator.GetMaxDuration();
  SetMove(currentScreen.GlobalRect().GlobalZero(), m_positionInterpolator.GetTargetPosition(), currentScreen);

  m_positionInterpolator.SetMinDuration(minDuration);
  m_positionInterpolator.SetMaxDuration(maxDuration);

  minDuration = m_scaleInterpolator.GetMinDuration();
  maxDuration = m_scaleInterpolator.GetMaxDuration();
  SetScale(currentScreen.GetScale(), m_scaleInterpolator.GetTargetScale());

  m_scaleInterpolator.SetMinDuration(minDuration);
  m_scaleInterpolator.SetMaxDuration(maxDuration);

  minDuration = m_angleInterpolator.GetMinDuration();
  maxDuration = m_angleInterpolator.GetMaxDuration();
  SetRotate(currentScreen.GetAngle(), m_angleInterpolator.GetTargetAngle());

  m_angleInterpolator.SetMinDuration(minDuration);
  m_angleInterpolator.SetMaxDuration(maxDuration);
}

void MapLinearAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos, ScreenBase const & convertor)
{
  m_positionInterpolator = PositionInterpolator(startPos, endPos, convertor);
  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Position);
}

void MapLinearAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos, m2::RectD const & viewportRect,
                                 double scale)
{
  m_positionInterpolator = PositionInterpolator(startPos, endPos, viewportRect, scale);
  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Position);
}

void MapLinearAnimation::SetRotate(double startAngle, double endAngle)
{
  m_angleInterpolator = AngleInterpolator(startAngle, endAngle);
  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Angle);
}

void MapLinearAnimation::SetScale(double startScale, double endScale)
{
  m_scaleInterpolator = ScaleInterpolator(startScale, endScale, false /* isAutoZoom */);
  if (m_scaleInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Scale);
}

Animation::TObjectProperties const & MapLinearAnimation::GetProperties(Object object) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());
  return m_properties;
}

bool MapLinearAnimation::HasProperty(Object object, ObjectProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void MapLinearAnimation::Advance(double elapsedSeconds)
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Advance(elapsedSeconds);

  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.Advance(elapsedSeconds);

  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.Advance(elapsedSeconds);
}

void MapLinearAnimation::Finish()
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Finish();

  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.Finish();

  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.Finish();

  Animation::Finish();
}

void MapLinearAnimation::SetMaxDuration(double maxDuration)
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.SetMaxDuration(maxDuration);

  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.SetMaxDuration(maxDuration);

  SetMaxScaleDuration(maxDuration);
}

void MapLinearAnimation::SetMinDuration(double minDuration)
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.SetMinDuration(minDuration);

  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.SetMinDuration(minDuration);

  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.SetMinDuration(minDuration);
}

void MapLinearAnimation::SetMaxScaleDuration(double maxDuration)
{
  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.SetMaxDuration(maxDuration);
}

double MapLinearAnimation::GetDuration() const
{
  double duration = 0.0;
  if (m_angleInterpolator.IsActive())
    duration = m_angleInterpolator.GetDuration();
  if (m_scaleInterpolator.IsActive())
    duration = std::max(duration, m_scaleInterpolator.GetDuration());
  if (m_positionInterpolator.IsActive())
    duration = std::max(duration, m_positionInterpolator.GetDuration());
  return duration;
}

double MapLinearAnimation::GetMaxDuration() const
{
  double maxDuration = Animation::kInvalidAnimationDuration;

  if (!Animation::GetMaxDuration(m_angleInterpolator, maxDuration) ||
      !Animation::GetMaxDuration(m_scaleInterpolator, maxDuration) ||
      !Animation::GetMaxDuration(m_positionInterpolator, maxDuration))
    return Animation::kInvalidAnimationDuration;

  return maxDuration;
}

double MapLinearAnimation::GetMinDuration() const
{
  double minDuration = Animation::kInvalidAnimationDuration;

  if (!Animation::GetMinDuration(m_angleInterpolator, minDuration) ||
      !Animation::GetMinDuration(m_scaleInterpolator, minDuration) ||
      !Animation::GetMinDuration(m_positionInterpolator, minDuration))
    return Animation::kInvalidAnimationDuration;

  return minDuration;
}

bool MapLinearAnimation::IsFinished() const
{
  return m_angleInterpolator.IsFinished() && m_scaleInterpolator.IsFinished() && m_positionInterpolator.IsFinished();
}

bool MapLinearAnimation::GetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

bool MapLinearAnimation::GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool MapLinearAnimation::GetProperty(Object object, ObjectProperty property, bool targetValue,
                                     PropertyValue & value) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());

  switch (property)
  {
  case Animation::ObjectProperty::Position:
    if (m_positionInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_positionInterpolator.GetTargetPosition()
                                        : m_positionInterpolator.GetPosition());
      return true;
    }
    return false;
  case Animation::ObjectProperty::Scale:
    if (m_scaleInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_scaleInterpolator.GetTargetScale() : m_scaleInterpolator.GetScale());
      return true;
    }
    return false;
  case Animation::ObjectProperty::Angle:
    if (m_angleInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_angleInterpolator.GetTargetAngle() : m_angleInterpolator.GetAngle());
      return true;
    }
    return false;
  default: ASSERT(false, ("Wrong property:", static_cast<int>(property)));
  }

  return false;
}

}  // namespace df
