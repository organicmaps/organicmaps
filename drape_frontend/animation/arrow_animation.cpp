#include "arrow_animation.hpp"

namespace  df
{

ArrowAnimation::ArrowAnimation(m2::PointD const & startPos, m2::PointD const & endPos, double moveDuration,
               double startAngle, double endAngle)
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
  , m_positionInterpolator(moveDuration, 0.0 /* delay */, startPos, endPos)
  , m_angleInterpolator(startAngle, endAngle)
{
  m_objects.insert(Animation::MyPositionArrow);
  if (m_positionInterpolator.IsActive())
    m_properties.insert(Animation::Position);
  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::Angle);
}

Animation::TAnimObjects const & ArrowAnimation::GetObjects() const
{
   return m_objects;
}

bool ArrowAnimation::HasObject(TObject object) const
{
  return object == Animation::MyPositionArrow;
}

Animation::TObjectProperties const & ArrowAnimation::GetProperties(TObject object) const
{
  return m_properties;
}

bool ArrowAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void ArrowAnimation::Advance(double elapsedSeconds)
{
  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.Advance(elapsedSeconds);

  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Advance(elapsedSeconds);
}

void ArrowAnimation::Finish()
{
  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.Finish();

  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Finish();
}

void ArrowAnimation::SetMaxDuration(double maxDuration)
{
  if (m_positionInterpolator.IsActive())
    m_positionInterpolator.SetMaxDuration(maxDuration);

  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.SetMaxDuration(maxDuration);
}

double ArrowAnimation::GetDuration() const
{
  double duration = 0.0;
  if (m_angleInterpolator.IsActive())
    duration = m_angleInterpolator.GetDuration();
  if (m_positionInterpolator.IsActive())
    duration = max(duration, m_positionInterpolator.GetDuration());
  return duration;
}

bool ArrowAnimation::IsFinished() const
{
  return m_positionInterpolator.IsFinished() && m_angleInterpolator.IsFinished();
}

bool ArrowAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT_EQUAL(object, Animation::MyPositionArrow, ());

  switch (property)
  {
  case Animation::Position:
    if (m_positionInterpolator.IsActive())
    {
      value = PropertyValue(m_positionInterpolator.GetPosition());
      return true;
    }
    return false;
  case Animation::Angle:
    if (m_angleInterpolator.IsActive())
    {
      value = PropertyValue(m_angleInterpolator.GetAngle());
      return true;
    }
    return false;
  default:
    ASSERT(false, ("Wrong property:", property));
  }

  return false;
}


} // namespace df
