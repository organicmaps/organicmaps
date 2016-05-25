#include "follow_animation.hpp"

#include "animation_system.hpp"

#include "base/assert.hpp"

namespace df
{

MapFollowAnimation::MapFollowAnimation(m2::PointD const & globalPosition,
                                       double startScale, double endScale,
                                       double startAngle, double endAngle,
                                       m2::PointD const & startPixelPosition,
                                       m2::PointD const & endPixelPosition,
                                       m2::RectD const & pixelRect)
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
  , m_scaleInterpolator(startScale, endScale)
  , m_pixelPosInterpolator(startPixelPosition, endPixelPosition, pixelRect)
  , m_angleInterpolator(startAngle, endAngle)
  , m_globalPosition(globalPosition)
{
  double const duration = CalculateDuration();
  m_scaleInterpolator.SetMinDuration(duration);
  m_angleInterpolator.SetMinDuration(duration);
  m_pixelPosInterpolator.SetMinDuration(duration);

  m_objects.insert(Animation::MapPlane);

  if (m_scaleInterpolator.IsActive())
    m_properties.insert(Animation::Scale);
  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::Angle);
  if (m_pixelPosInterpolator.IsActive())
    m_properties.insert(Animation::Position);
}

Animation::TObjectProperties const & MapFollowAnimation::GetProperties(TObject object) const
{
  ASSERT_EQUAL(object, Animation::MapPlane, ());
  return m_properties;
}

bool MapFollowAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void MapFollowAnimation::Advance(double elapsedSeconds)
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Advance(elapsedSeconds);
  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.Advance(elapsedSeconds);
  if (m_pixelPosInterpolator.IsActive())
    m_pixelPosInterpolator.Advance(elapsedSeconds);
}

void MapFollowAnimation::Finish()
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Finish();
  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.Finish();
  if (m_pixelPosInterpolator.IsActive())
    m_pixelPosInterpolator.Finish();
  Animation::Finish();
}

void MapFollowAnimation::SetMaxDuration(double maxDuration)
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.SetMaxDuration(maxDuration);
  if (m_scaleInterpolator.IsActive())
    m_scaleInterpolator.SetMaxDuration(maxDuration);
  if (m_pixelPosInterpolator.IsActive())
    m_pixelPosInterpolator.SetMaxDuration(maxDuration);
}

double MapFollowAnimation::GetDuration() const
{
  return CalculateDuration();
}

double MapFollowAnimation::CalculateDuration() const
{
  return max(max(m_angleInterpolator.GetDuration(),
                 m_angleInterpolator.GetDuration()), m_scaleInterpolator.GetDuration());
}

bool MapFollowAnimation::IsFinished() const
{
  return m_pixelPosInterpolator.IsFinished() && m_angleInterpolator.IsFinished() &&
         m_scaleInterpolator.IsFinished();
}

// static
m2::PointD MapFollowAnimation::CalculateCenter(ScreenBase const & screen, m2::PointD const & userPos,
                                               m2::PointD const & pixelPos, double azimuth)
{
  double const scale = screen.GlobalRect().GetLocalRect().SizeX() / screen.PixelRect().SizeX();
  return CalculateCenter(scale, screen.PixelRect(), userPos, pixelPos, azimuth);
}

// static
m2::PointD MapFollowAnimation::CalculateCenter(double scale, m2::RectD const & pixelRect,
                                               m2::PointD const & userPos, m2::PointD const & pixelPos,
                                               double azimuth)
{
  m2::PointD formingVector = (pixelRect.Center() - pixelPos) * scale;
  formingVector.y = -formingVector.y;
  formingVector.Rotate(azimuth);
  return userPos + formingVector;
}

bool MapFollowAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  if (property == Animation::Position)
  {
    m2::RectD const pixelRect = AnimationSystem::Instance().GetLastScreen().PixelRect();
    value = PropertyValue(CalculateCenter(m_scaleInterpolator.GetScale(), pixelRect, m_globalPosition,
                          m_pixelPosInterpolator.GetPosition(), m_angleInterpolator.GetAngle()));
    return true;
  }
  if (property == Animation::Angle)
  {
    value = PropertyValue(m_angleInterpolator.GetAngle());
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

bool MapFollowAnimation::HasScale() const
{
  return m_scaleInterpolator.IsActive();
}

bool MapFollowAnimation::HasPixelOffset() const
{
  return m_pixelPosInterpolator.IsActive();
}

} // namespace df
