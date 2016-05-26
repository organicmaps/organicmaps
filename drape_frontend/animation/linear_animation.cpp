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
  , m_scaleInterpolator(startScale, endScale)
{
  m_objects.insert(Animation::MapPlane);

  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::Position);

  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::Angle);

  if (m_scaleInterpolator.IsActive())
    m_properties.insert(Animation::Scale);
}

MapLinearAnimation::MapLinearAnimation()
  : Animation(true /* couldBeInterrupted */, false /* couldBeBlended */)
{
  m_objects.insert(Animation::MapPlane);
}

void MapLinearAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos,
                                 ScreenBase const & convertor)
{
  m_positionInterpolator = PositionInterpolator(startPos, endPos, convertor);
  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::Position);
}

void MapLinearAnimation::SetRotate(double startAngle, double endAngle)
{
  m_angleInterpolator = AngleInterpolator(startAngle, endAngle);
  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::Angle);
}

void MapLinearAnimation::SetScale(double startScale, double endScale)
{
  m_scaleInterpolator = ScaleInterpolator(startScale, endScale);
  if (m_scaleInterpolator.IsActive())
    m_properties.insert(Animation::Scale);
}

Animation::TObjectProperties const & MapLinearAnimation::GetProperties(TObject object) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());
  return m_properties;
}

bool MapLinearAnimation::HasProperty(TObject object, TProperty property) const
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

bool MapLinearAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

bool MapLinearAnimation::GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool MapLinearAnimation::GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());

  switch (property)
  {
  case Animation::Position:
    if (m_positionInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_positionInterpolator.GetTargetPosition() : m_positionInterpolator.GetPosition());
      return true;
    }
    return false;
  case Animation::Scale:
    if (m_scaleInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_scaleInterpolator.GetTargetScale() : m_scaleInterpolator.GetScale());
      return true;
    }
    return false;
  case Animation::Angle:
    if (m_angleInterpolator.IsActive())
    {
      value = PropertyValue(targetValue ? m_angleInterpolator.GetTargetAngle() : m_angleInterpolator.GetAngle());
      return true;
    }
    return false;
  default:
    ASSERT(false, ("Wrong property:", property));
  }

  return false;
}

} // namespace df
