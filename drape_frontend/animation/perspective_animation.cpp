#include "perspective_animation.hpp"

namespace df
{

PerspectiveSwitchAnimation::PerspectiveSwitchAnimation(double startAngle, double endAngle, double angleFOV)
  : Animation(false /* couldBeInterrupted */, false /* couldBeBlended */)
  , m_angleInterpolator(GetRotateDuration(startAngle, endAngle), startAngle, endAngle)
  , m_startAngle(startAngle)
  , m_endAngle(endAngle)
  , m_angleFOV(angleFOV)
  , m_isEnablePerspectiveAnim(m_endAngle > 0.0)
  , m_needPerspectiveSwitch(false)
{
  m_objects.insert(Animation::MapPlane);
  m_properties.insert(Animation::AnglePerspective);
  m_properties.insert(Animation::SwitchPerspective);
}

// static
double PerspectiveSwitchAnimation::GetRotateDuration(double startAngle, double endAngle)
{
  double const kScalar = 0.5;
  return kScalar * fabs(endAngle - startAngle) / math::pi4;
}

Animation::TObjectProperties const & PerspectiveSwitchAnimation::GetProperties(TObject object) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());
  return m_properties;
}

bool PerspectiveSwitchAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void PerspectiveSwitchAnimation::Advance(double elapsedSeconds)
{
  m_angleInterpolator.Advance(elapsedSeconds);
}

void PerspectiveSwitchAnimation::Finish()
{
  m_angleInterpolator.Finish();
  Animation::Finish();
}

void PerspectiveSwitchAnimation::OnStart()
{
  if (m_isEnablePerspectiveAnim)
    m_needPerspectiveSwitch = true;
  Animation::OnStart();
}

void PerspectiveSwitchAnimation::OnFinish()
{
  if (!m_isEnablePerspectiveAnim)
    m_needPerspectiveSwitch = true;
  Animation::OnFinish();
}

void PerspectiveSwitchAnimation::SetMaxDuration(double maxDuration)
{
  m_angleInterpolator.SetMaxDuration(maxDuration);
}

double PerspectiveSwitchAnimation::GetDuration() const
{
  return m_angleInterpolator.GetDuration();
}

bool PerspectiveSwitchAnimation::IsFinished() const
{
  return m_angleInterpolator.IsFinished();
}

bool PerspectiveSwitchAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());

  switch (property)
  {
  case Animation::AnglePerspective:
    value = PropertyValue(m_angleInterpolator.GetAngle());
    return true;
  case Animation::SwitchPerspective:
    if (m_needPerspectiveSwitch)
    {
      m_needPerspectiveSwitch = false;
      value = PropertyValue(SwitchPerspectiveParams(m_isEnablePerspectiveAnim, m_startAngle, m_endAngle, m_angleFOV));
      return true;
    }
    return false;
  default:
    ASSERT(false, ("Wrong property:", property));
  }

  return false;
}

} // namespace df
