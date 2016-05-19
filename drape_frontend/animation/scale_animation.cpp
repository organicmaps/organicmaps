#include "scale_animation.hpp"

#include "animation_system.hpp"

#include "base/assert.hpp"

namespace df
{

MapScaleAnimation::MapScaleAnimation(double startScale, double endScale,
                                     m2::PointD const & globalPosition, m2::PointD const & offset)
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
  , m_scaleInterpolator(startScale, endScale)
  , m_pixelOffset(offset)
  , m_globalPosition(globalPosition)
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

bool MapScaleAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  if (property == Animation::Position)
  {
    ScreenBase screen = AnimationSystem::Instance().GetLastScreen();
    screen.SetScale(m_scaleInterpolator.GetScale());
    value = PropertyValue(screen.PtoG(screen.GtoP(m_globalPosition) + m_pixelOffset));
    return true;
  }
  if (property == Animation::Scale)
  {
    value = PropertyValue(m_scaleInterpolator.GetScale());
    return true;
  }
  ASSERT(false, ("Wrong property:", property));
  return false;
}

} // namespace df
