#include "follow_animation.hpp"

#include "animation_constants.hpp"
#include "animation_system.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace df
{

MapFollowAnimation::MapFollowAnimation(ScreenBase const & screen,
                                       m2::PointD const & globalUserPosition,
                                       m2::PointD const & endPixelPosition,
                                       double startScale, double endScale,
                                       double startAngle, double endAngle,
                                       bool isAutoZoom)
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
  , m_isAutoZoom(isAutoZoom)
  , m_scaleInterpolator(startScale, endScale, m_isAutoZoom)
  , m_angleInterpolator(startAngle, endAngle)
  , m_globalPosition(globalUserPosition)
  , m_endPixelPosition(endPixelPosition)
{
  m_offset = screen.PtoG(screen.P3dtoP(m_endPixelPosition)) - m_globalPosition;
  double const scale = m_isAutoZoom ? screen.GetScale() : (startScale + endScale) / 2.0;
  double const moveDuration = PositionInterpolator::GetMoveDuration(m_offset.Length(), screen.PixelRectIn3d(), scale);

  m_offsetInterpolator = PositionInterpolator(moveDuration, 0.0, m_offset, m2::PointD(0.0, 0.0));

  double const duration = CalculateDuration();

  m_scaleInterpolator.SetMinDuration(duration);
  m_angleInterpolator.SetMinDuration(duration);

  m_objects.insert(Animation::MapPlane);

  if (m_scaleInterpolator.IsActive())
    m_properties.insert(Animation::Scale);
  if (m_angleInterpolator.IsActive())
    m_properties.insert(Animation::Angle);
  if (m_offsetInterpolator.IsActive() || m_scaleInterpolator.IsActive() || m_angleInterpolator.IsActive())
    m_properties.insert(Animation::Position);

  // If MapFollowAnimation affects only angles, disable rewinding.
  SetCouldBeRewinded(!m_angleInterpolator.IsActive() || m_scaleInterpolator.IsActive() ||
                     m_offsetInterpolator.IsActive());
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
  if (m_offsetInterpolator.IsActive())
    m_offsetInterpolator.Advance(elapsedSeconds);
}

void MapFollowAnimation::Finish()
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.Finish();
  if (m_scaleInterpolator.IsActive())
  {
    if (m_isAutoZoom)
      m_scaleInterpolator.SetActive(false);
    else
      m_scaleInterpolator.Finish();
  }
  if (m_offsetInterpolator.IsActive())
    m_offsetInterpolator.Finish();
  Animation::Finish();
}

void MapFollowAnimation::SetMaxDuration(double maxDuration)
{
  if (m_angleInterpolator.IsActive())
    m_angleInterpolator.SetMaxDuration(maxDuration);
  if (!m_isAutoZoom && m_scaleInterpolator.IsActive())
    m_scaleInterpolator.SetMaxDuration(maxDuration);
  if (m_offsetInterpolator.IsActive())
    m_offsetInterpolator.SetMaxDuration(maxDuration);
}

double MapFollowAnimation::GetDuration() const
{
  return CalculateDuration();
}

double MapFollowAnimation::CalculateDuration() const
{
  double duration = max(m_angleInterpolator.GetDuration(), m_offsetInterpolator.GetDuration());
  if (!m_isAutoZoom)
    duration = max(duration, m_scaleInterpolator.GetDuration());
  return duration;
}

bool MapFollowAnimation::IsFinished() const
{
  return m_angleInterpolator.IsFinished() && m_scaleInterpolator.IsFinished() && m_offsetInterpolator.IsFinished();
}

bool MapFollowAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, false /* targetValue */, value);
}

bool MapFollowAnimation::GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  return GetProperty(object, property, true /* targetValue */, value);
}

bool MapFollowAnimation::GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const
{
  if (property == Animation::Position)
  {
    ScreenBase tmp = AnimationSystem::Instance().GetLastScreen();
    if (targetValue)
    {
      tmp.SetFromParams(m_globalPosition, m_angleInterpolator.GetTargetAngle(), m_isAutoZoom ? m_scaleInterpolator.GetScale()
                                                                                             : m_scaleInterpolator.GetTargetScale());
      tmp.MatchGandP3d(m_globalPosition, m_endPixelPosition);
    }
    else
    {
      double const scale = m_scaleInterpolator.GetScale() / m_scaleInterpolator.GetStartScale();
      double const angle = m_angleInterpolator.GetAngle() - m_angleInterpolator.GetStartAngle();
      m2::PointD offset = m_offsetInterpolator.GetPosition() * scale;
      offset.Rotate(angle);
      m2::PointD pos = m_globalPosition + offset;

      tmp.SetFromParams(m_globalPosition, m_angleInterpolator.GetAngle(), m_scaleInterpolator.GetScale());
      tmp.MatchGandP3d(pos, m_endPixelPosition);
    }
    value = PropertyValue(tmp.GetOrg());
    return true;
  }
  if (property == Animation::Angle)
  {
    value = PropertyValue(targetValue ? m_angleInterpolator.GetTargetAngle() : m_angleInterpolator.GetAngle());
    return true;
  }
  if (property == Animation::Scale)
  {
    value = PropertyValue((targetValue && !m_isAutoZoom) ? m_scaleInterpolator.GetTargetScale()
                                                         : m_scaleInterpolator.GetScale());
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
  return m_offsetInterpolator.IsActive();
}

} // namespace df
