#include "scale_animation.hpp"

#include "animation_system.hpp"

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
  m_objects.insert(Animation::MapPlane);
  m_properties.insert(Animation::Scale);
  m_properties.insert(Animation::Position);
}

Animation::TObjectProperties const & MapScaleAnimation::GetProperties(TObject object) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());
  return m_properties;
}

bool MapScaleAnimation::HasProperty(TObject object, TProperty property) const
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

double MapScaleAnimation::GetDuration() const
{
  return m_scaleInterpolator.GetDuration();
}

bool MapScaleAnimation::IsFinished() const
{
  return m_scaleInterpolator.IsFinished();
}

bool MapScaleAnimation::GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());

  if (property == Animation::Position)
  {
    ScreenBase screen = AnimationSystem::Instance().GetLastScreen();
    screen.SetScale(targetValue ? m_scaleInterpolator.GetTargetScale() : m_scaleInterpolator.GetScale());
    m2::PointD const pixelOffset = screen.PixelRect().Center() - screen.P3dtoP(m_pxScaleCenter);
    value = PropertyValue(screen.PtoG(screen.GtoP(m_globalScaleCenter) + pixelOffset));
    return true;
  }
  if (property == Animation::Scale)
  {
    value = PropertyValue(targetValue ? m_scaleInterpolator.GetTargetScale() : m_scaleInterpolator.GetScale());
    return true;
  }
  ASSERT(false, ("Wrong property:", property));
  return false;
}

bool MapScaleAnimation::GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool MapScaleAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

} // namespace df
