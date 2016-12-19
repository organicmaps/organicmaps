#include "linear_animation.hpp"

#include "base/assert.hpp"

namespace df
{

MapLinearAnimation::MapLinearAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                                       double startAngle, double endAngle,
                                       double startScale, double endScale, ScreenBase const & convertor)
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

MapLinearAnimation::MapLinearAnimation()
  : Animation(true /* couldBeInterrupted */, false /* couldBeBlended */)
{
  m_objects.insert(Animation::Object::MapPlane);
}

void MapLinearAnimation::Init(ScreenBase const & screen, TPropertyCache const & properties)
{
  ScreenBase currentScreen;
  GetCurrentScreen(properties, screen, currentScreen);

  SetMove(currentScreen.GlobalRect().GlobalZero(), m_positionInterpolator.GetTargetPosition(), currentScreen);
  SetScale(currentScreen.GetScale(), m_scaleInterpolator.GetTargetScale());
  SetRotate(currentScreen.GetAngle(), m_angleInterpolator.GetTargetAngle());
}

void MapLinearAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos,
                                 ScreenBase const & convertor)
{
  m_positionInterpolator = PositionInterpolator(startPos, endPos, convertor);
  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::ObjectProperty::Position);
}

void MapLinearAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos,
                                 m2::RectD const & viewportRect, double scale)
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
    duration = max(duration, m_scaleInterpolator.GetDuration());
  if (m_positionInterpolator.IsActive())
    duration = max(duration, m_positionInterpolator.GetDuration());
  return duration;
}

bool MapLinearAnimation::IsFinished() const
{
  return m_angleInterpolator.IsFinished() && m_scaleInterpolator.IsFinished() &&
         m_positionInterpolator.IsFinished();
}

bool MapLinearAnimation::GetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

bool MapLinearAnimation::GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool MapLinearAnimation::GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());

  switch (property)
  {
  case Animation::ObjectProperty::Position:
    if (m_positionInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_positionInterpolator.GetTargetPosition() : m_positionInterpolator.GetPosition());
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
  default:
    ASSERT(false, ("Wrong property:", static_cast<int>(property)));
  }

  return false;
}

} // namespace df
