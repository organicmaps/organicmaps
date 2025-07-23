#include "scale_animation.hpp"

#include "drape_frontend/animation_system.hpp"

#include "base/assert.hpp"

namespace df
{

MapScaleAnimation::MapScaleAnimation(double startScale, double endScale, m2::PointD const & globalScaleCenter,
                                     m2::PointD const & pxScaleCenter)
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
  , m_scaleInterpolator(startScale, endScale, false /* isAutoZoom */)
  , m_pxScaleCenter(pxScaleCenter)
  , m_globalScaleCenter(globalScaleCenter)
{
  m_objects.insert(Animation::Object::MapPlane);
  m_properties.insert(Animation::ObjectProperty::Scale);
  m_properties.insert(Animation::ObjectProperty::Position);
}

void MapScaleAnimation::Init(ScreenBase const & screen, TPropertyCache const & properties)
{
  ScreenBase currentScreen;
  GetCurrentScreen(properties, screen, currentScreen);

  double const minDuration = m_scaleInterpolator.GetMinDuration();
  double const maxDuration = m_scaleInterpolator.GetMaxDuration();

  m_scaleInterpolator =
      ScaleInterpolator(currentScreen.GetScale(), m_scaleInterpolator.GetTargetScale(), false /* isAutoZoom */);

  m_scaleInterpolator.SetMinDuration(minDuration);
  m_scaleInterpolator.SetMaxDuration(maxDuration);
}

Animation::TObjectProperties const & MapScaleAnimation::GetProperties(Object object) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());
  return m_properties;
}

bool MapScaleAnimation::HasProperty(Object object, ObjectProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void MapScaleAnimation::Advance(double elapsedSeconds)
{
  m_scaleInterpolator.Advance(elapsedSeconds);
}

void MapScaleAnimation::Finish()
{
  m_scaleInterpolator.Finish();
  Animation::Finish();
}

void MapScaleAnimation::SetMaxDuration(double maxDuration)
{
  m_scaleInterpolator.SetMaxDuration(maxDuration);
}

void MapScaleAnimation::SetMinDuration(double minDuration)
{
  m_scaleInterpolator.SetMinDuration(minDuration);
}

double MapScaleAnimation::GetDuration() const
{
  return m_scaleInterpolator.GetDuration();
}

double MapScaleAnimation::GetMaxDuration() const
{
  return m_scaleInterpolator.GetMaxDuration();
}

double MapScaleAnimation::GetMinDuration() const
{
  return m_scaleInterpolator.GetMinDuration();
}

bool MapScaleAnimation::IsFinished() const
{
  return m_scaleInterpolator.IsFinished();
}

bool MapScaleAnimation::GetProperty(Object object, ObjectProperty property, bool targetValue,
                                    PropertyValue & value) const
{
  ASSERT_EQUAL(static_cast<int>(object), static_cast<int>(Animation::Object::MapPlane), ());

  if (property == Animation::ObjectProperty::Position)
  {
    ScreenBase screen = AnimationSystem::Instance().GetLastScreen();
    screen.SetScale(targetValue ? m_scaleInterpolator.GetTargetScale() : m_scaleInterpolator.GetScale());
    m2::PointD const pixelOffset = screen.PixelRect().Center() - screen.P3dtoP(m_pxScaleCenter);
    value = PropertyValue(screen.PtoG(screen.GtoP(m_globalScaleCenter) + pixelOffset));
    return true;
  }
  if (property == Animation::ObjectProperty::Scale)
  {
    value = PropertyValue(targetValue ? m_scaleInterpolator.GetTargetScale() : m_scaleInterpolator.GetScale());
    return true;
  }
  ASSERT(false, ("Wrong property:", static_cast<int>(property)));
  return false;
}

bool MapScaleAnimation::GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool MapScaleAnimation::GetProperty(Object object, ObjectProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

}  // namespace df
