#include "animation_system.hpp"
#include "animation/interpolations.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

double CalcAnimSpeedDuration(double pxDiff, double pxSpeed)
{
  double const kEps = 1e-5;

  if (my::AlmostEqualAbs(pxDiff, 0.0, kEps))
    return 0.0;

  return fabs(pxDiff) / pxSpeed;
}

class PropertyBlender
{
public:
  PropertyBlender() = default;

  void Blend(Animation::PropertyValue const & value)
  {
    // Now perspective parameters can't be blended.
    if (value.m_type == Animation::PropertyValue::ValuePerspectiveParams)
    {
      m_value = value;
      m_counter = 1;
      return;
    }

    if (m_counter != 0)
    {
      // New value type resets current blended value.
      if (m_value.m_type != value.m_type)
      {
        m_value = value;
        m_counter = 1;
        return;
      }

      if (value.m_type == Animation::PropertyValue::ValueD)
        m_value.m_valueD += value.m_valueD;
      else if (value.m_type == Animation::PropertyValue::ValuePointD)
        m_value.m_valuePointD += value.m_valuePointD;
    }
    else
    {
      m_value = value;
    }
    m_counter++;
  }

  Animation::PropertyValue Finish()
  {
    if (m_counter == 0)
      return m_value;

    double const scalar = 1.0 / m_counter;
    m_counter = 0;
    if (m_value.m_type == Animation::PropertyValue::ValueD)
      m_value.m_valueD *= scalar;
    else if (m_value.m_type == Animation::PropertyValue::ValuePointD)
      m_value.m_valuePointD *= scalar;

    return m_value;
  }

  bool IsEmpty() const
  {
    return m_counter == 0;
  }

private:
  Animation::PropertyValue m_value;
  uint32_t m_counter = 0;
};

} // namespace

bool Animation::CouldBeBlendedWith(Animation const & animation) const
{
  return (GetType() != animation.GetType()) &&
         m_couldBeBlended && animation.m_couldBeBlended;
}

Interpolator::Interpolator(double duration, double delay)
  : m_elapsedTime(0.0)
  , m_duration(duration)
  , m_delay(delay)
  , m_isActive(false)
{
  ASSERT_GREATER_OR_EQUAL(m_duration, 0.0, ());
}

bool Interpolator::IsFinished() const
{
  if (!IsActive())
    return true;

  return m_elapsedTime > (m_duration + m_delay);
}

void Interpolator::Advance(double elapsedSeconds)
{
  m_elapsedTime += elapsedSeconds;
}

void Interpolator::Finish()
{
  m_elapsedTime = m_duration + m_delay + 1.0;
}

bool Interpolator::IsActive() const
{
  return m_isActive;
}

void Interpolator::SetActive(bool active)
{
  m_isActive = active;
}

void Interpolator::SetMaxDuration(double maxDuration)
{
  m_duration = min(m_duration, maxDuration);
}

void Interpolator::SetMinDuration(double minDuration)
{
  m_duration = max(m_duration, minDuration);
}

double Interpolator::GetT() const
{
  if (IsFinished())
    return 1.0;

  return max(m_elapsedTime - m_delay, 0.0) / m_duration;
}

double Interpolator::GetElapsedTime() const
{
  return m_elapsedTime;
}

double Interpolator::GetDuration() const
{
  return m_duration;
}

PositionInterpolator::PositionInterpolator()
  : PositionInterpolator(0.0 /* duration */, 0.0 /* delay */, m2::PointD(), m2::PointD())
{}

PositionInterpolator::PositionInterpolator(double duration, double delay,
                                           m2::PointD const & startPosition,
                                           m2::PointD const & endPosition)
  : Interpolator(duration, delay)
  , m_startPosition(startPosition)
  , m_endPosition(endPosition)
  , m_position(startPosition)
{}

PositionInterpolator::PositionInterpolator(m2::PointD const & startPosition,
                                           m2::PointD const & endPosition,
                                           ScreenBase const & convertor)
  : PositionInterpolator(0.0 /* delay */, startPosition, endPosition, convertor)
{}

PositionInterpolator::PositionInterpolator(double delay, m2::PointD const & startPosition,
                                           m2::PointD const & endPosition,
                                           ScreenBase const & convertor)
  : Interpolator(PositionInterpolator::GetMoveDuration(startPosition, endPosition, convertor), delay)
  , m_startPosition(startPosition)
  , m_endPosition(endPosition)
  , m_position(startPosition)
{
  SetActive(m_startPosition != m_endPosition);
}

PositionInterpolator::PositionInterpolator(m2::PointD const & startPosition,
                                           m2::PointD const & endPosition,
                                           m2::RectD const & pixelRect)
  : PositionInterpolator(0.0 /* delay */, startPosition, endPosition, pixelRect)
{}

PositionInterpolator::PositionInterpolator(double delay, m2::PointD const & startPosition,
                                           m2::PointD const & endPosition, m2::RectD const & pixelRect)
  : Interpolator(PositionInterpolator::GetPixelMoveDuration(startPosition, endPosition, pixelRect), delay)
  , m_startPosition(startPosition)
  , m_endPosition(endPosition)
  , m_position(startPosition)
{
  SetActive(m_startPosition != m_endPosition);
}

//static
double PositionInterpolator::GetMoveDuration(m2::PointD const & startPosition,
                                             m2::PointD const & endPosition,
                                             ScreenBase const & convertor)
{
  return GetPixelMoveDuration(convertor.GtoP(startPosition),
                              convertor.GtoP(endPosition),
                              convertor.PixelRectIn3d());
}

double PositionInterpolator::GetPixelMoveDuration(m2::PointD const & startPosition,
                                                  m2::PointD const & endPosition,
                                                  m2::RectD const & pixelRect)
{
  double const kMinMoveDuration = 0.2;
  double const kMinSpeedScalar = 0.2;
  double const kMaxSpeedScalar = 7.0;
  double const kEps = 1e-5;

  double const pixelLength = endPosition.Length(startPosition);
  if (pixelLength < kEps)
    return 0.0;

  double const minSize = min(pixelRect.SizeX(), pixelRect.SizeY());
  if (pixelLength < kMinSpeedScalar * minSize)
    return kMinMoveDuration;

  double const pixelSpeed = kMaxSpeedScalar * minSize;
  return CalcAnimSpeedDuration(pixelLength, pixelSpeed);
}

void PositionInterpolator::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_position = InterpolatePoint(m_startPosition, m_endPosition, GetT());
}

void PositionInterpolator::Finish()
{
  TBase::Finish();
  m_position = m_endPosition;
}

ScaleInterpolator::ScaleInterpolator()
  : ScaleInterpolator(1.0 /* startScale */, 1.0 /* endScale */)
{}

ScaleInterpolator::ScaleInterpolator(double startScale, double endScale)
  : ScaleInterpolator(0.0 /* delay */, startScale, endScale)
{}

ScaleInterpolator::ScaleInterpolator(double delay, double startScale, double endScale)
  : Interpolator(ScaleInterpolator::GetScaleDuration(startScale, endScale), delay)
  , m_startScale(startScale)
  , m_endScale(endScale)
  , m_scale(startScale)
{
  SetActive(m_startScale != m_endScale);
}

// static
double ScaleInterpolator::GetScaleDuration(double startScale, double endScale)
{
  // Resize 2.0 times should be done for 0.2 seconds.
  double constexpr kPixelSpeed = 2.0 / 0.2;

  if (startScale > endScale)
    swap(startScale, endScale);

  return CalcAnimSpeedDuration(endScale / startScale, kPixelSpeed);
}

void ScaleInterpolator::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_scale = InterpolateDouble(m_startScale, m_endScale, GetT());
}

void ScaleInterpolator::Finish()
{
  TBase::Finish();
  m_scale = m_endScale;
}

AngleInterpolator::AngleInterpolator()
  : AngleInterpolator(0.0 /* startAngle */, 0.0 /* endAngle */)
{}

AngleInterpolator::AngleInterpolator(double startAngle, double endAngle)
  : AngleInterpolator(0.0 /* delay */, startAngle, endAngle)
{}

AngleInterpolator::AngleInterpolator(double delay, double startAngle, double endAngle)
  : Interpolator(AngleInterpolator::GetRotateDuration(startAngle, endAngle), delay)
  , m_startAngle(ang::AngleIn2PI(startAngle))
  , m_endAngle(ang::AngleIn2PI(endAngle))
  , m_angle(m_startAngle)
{
  SetActive(m_startAngle != m_endAngle);
}

AngleInterpolator::AngleInterpolator(double delay, double duration, double startAngle, double endAngle)
  : Interpolator(duration, delay)
  , m_startAngle(ang::AngleIn2PI(startAngle))
  , m_endAngle(ang::AngleIn2PI(endAngle))
  , m_angle(m_startAngle)
{
  SetActive(m_startAngle != m_endAngle);
}

// static
double AngleInterpolator::GetRotateDuration(double startAngle, double endAngle)
{
  double const kRotateDurationScalar = 0.75;
  startAngle = ang::AngleIn2PI(startAngle);
  endAngle = ang::AngleIn2PI(endAngle);
  return kRotateDurationScalar * fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
}

void AngleInterpolator::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_angle = m_startAngle + ang::GetShortestDistance(m_startAngle, m_endAngle) * GetT();
}

void AngleInterpolator::Finish()
{
  TBase::Finish();
  m_angle = m_endAngle;
}

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
  ASSERT_EQUAL(object, Animation::MapPlane, ());

  switch (property)
  {
  case Animation::Position:
    if (m_positionInterpolator.IsActive())
    {
      value = PropertyValue(m_positionInterpolator.GetPosition());
      return true;
    }
    return false;
  case Animation::Scale:
    if (m_scaleInterpolator.IsActive())
    {
      value = PropertyValue(m_scaleInterpolator.GetScale());
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
  m_properties.insert(Animation::Scale);
  m_properties.insert(Animation::Angle);
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
  m_angleInterpolator.Advance(elapsedSeconds);
  m_scaleInterpolator.Advance(elapsedSeconds);
  m_pixelPosInterpolator.Advance(elapsedSeconds);
}

void MapFollowAnimation::Finish()
{
  m_angleInterpolator.Finish();
  m_scaleInterpolator.Finish();
  m_pixelPosInterpolator.Finish();
  Animation::Finish();
}

void MapFollowAnimation::SetMaxDuration(double maxDuration)
{
  m_angleInterpolator.SetMaxDuration(maxDuration);
  m_scaleInterpolator.SetMaxDuration(maxDuration);
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

PerspectiveSwitchAnimation::PerspectiveSwitchAnimation(double startAngle, double endAngle, double angleFOV)
  : Animation(false /* couldBeInterrupted */, true /* couldBeBlended */)
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

ParallelAnimation::ParallelAnimation()
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
{}

Animation::TObjectProperties const & ParallelAnimation::GetProperties(TObject object) const
{
  ASSERT(HasObject(object), ());
  return m_properties.find(object)->second;
}

bool ParallelAnimation::HasProperty(TObject object, TProperty property) const
{
  if (!HasObject(object))
    return false;
  TObjectProperties properties = GetProperties(object);
  return properties.find(property) != properties.end();
}

void ParallelAnimation::AddAnimation(drape_ptr<Animation> animation)
{
  TAnimObjects const & objects = animation->GetObjects();
  m_objects.insert(objects.begin(), objects.end());
  for (auto const & object : objects)
  {
    TObjectProperties const & properties = animation->GetProperties(object);
    m_properties[object].insert(properties.begin(), properties.end());
  }
  m_animations.push_back(move(animation));
}

void ParallelAnimation::OnStart()
{
  for (auto & anim : m_animations)
    anim->OnStart();
}

void ParallelAnimation::OnFinish()
{
  for (auto & anim : m_animations)
    anim->OnFinish();
}

void ParallelAnimation::Advance(double elapsedSeconds)
{
  auto iter = m_animations.begin();
  while (iter != m_animations.end())
  {
    (*iter)->Advance(elapsedSeconds);
    if ((*iter)->IsFinished())
    {
      (*iter)->OnFinish();
      iter = m_animations.erase(iter);
    }
    else
    {
      ++iter;
    }
  }
}

void ParallelAnimation::Finish()
{
  for (auto & anim : m_animations)
    anim->Finish();
  Animation::Finish();
}

SequenceAnimation::SequenceAnimation()
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
{
}

Animation::TAnimObjects const & SequenceAnimation::GetObjects() const
{
  return m_objects;
}

bool SequenceAnimation::HasObject(TObject object) const
{
  ASSERT(!m_animations.empty(), ());
  return m_animations.front()->HasObject(object);
}

Animation::TObjectProperties const & SequenceAnimation::GetProperties(TObject object) const
{
  ASSERT(HasObject(object), ());
  return m_properties.find(object)->second;
}

bool SequenceAnimation::HasProperty(TObject object, TProperty property) const
{
  ASSERT(!m_animations.empty(), ());
  return m_animations.front()->HasProperty(object, property);
}

void SequenceAnimation::SetMaxDuration(double maxDuration)
{
  ASSERT(false, ("Not implemented"));
}

double SequenceAnimation::GetDuration() const
{
  double duration = 0.0;
  for (auto const & anim : m_animations)
    duration += anim->GetDuration();
  return duration;
}

bool SequenceAnimation::IsFinished() const
{
  return m_animations.empty();
}

bool SequenceAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT(!m_animations.empty(), ());
  return m_animations.front()->GetProperty(object, property, value);
}

void SequenceAnimation::AddAnimation(drape_ptr<Animation> animation)
{
  m_animations.push_back(move(animation));
  if (m_animations.size() == 1)
    ObtainObjectProperties();
}

void SequenceAnimation::OnStart()
{
  if (m_animations.empty())
    return;
  m_animations.front()->OnStart();
  Animation::OnStart();
}

void SequenceAnimation::OnFinish()
{
  Animation::OnFinish();
}

void SequenceAnimation::Advance(double elapsedSeconds)
{
  if (m_animations.empty())
    return;
  m_animations.front()->Advance(elapsedSeconds);
  if (m_animations.front()->IsFinished())
  {
    m_animations.front()->OnFinish();
    AnimationSystem::Instance().SaveAnimationResult(*m_animations.front());
    m_animations.pop_front();
    ObtainObjectProperties();
  }
}

void SequenceAnimation::Finish()
{
  for (auto & anim : m_animations)
    anim->Finish();
  AnimationSystem::Instance().SaveAnimationResult(*m_animations.back());
  m_animations.clear();
  ObtainObjectProperties();
  Animation::Finish();
}

void SequenceAnimation::ObtainObjectProperties()
{
  m_objects.clear();
  m_properties.clear();

  if (m_animations.empty())
    return;

  TAnimObjects const & objects = m_animations.front()->GetObjects();
  m_objects.insert(objects.begin(), objects.end());
  for (auto const & object : objects)
  {
    TObjectProperties const & properties = m_animations.front()->GetProperties(object);
    m_properties[object].insert(properties.begin(), properties.end());
  }
}

bool AnimationSystem::GetRect(ScreenBase const & currentScreen, m2::AnyRectD & rect)
{
  m_lastScreen = currentScreen;

  double scale = currentScreen.GetScale();
  double angle = currentScreen.GetAngle();
  m2::PointD pos = currentScreen.GlobalRect().GlobalZero();

  Animation::PropertyValue value;
  if (GetProperty(Animation::MapPlane, Animation::Scale, value))
    scale = value.m_valueD;

  if (GetProperty(Animation::MapPlane, Animation::Angle, value))
    angle = value.m_valueD;

  if (GetProperty(Animation::MapPlane, Animation::Position, value))
    pos = value.m_valuePointD;

  m2::RectD localRect = currentScreen.PixelRect();
  localRect.Offset(-localRect.Center());
  localRect.Scale(scale);
  rect = m2::AnyRectD(pos, angle, localRect);

  return true;
}

bool AnimationSystem::GetPerspectiveAngle(double & angle)
{
  Animation::PropertyValue value;
  if (GetProperty(Animation::MapPlane, Animation::AnglePerspective, value))
  {
    angle = value.m_valueD;
    return true;
  }
  return false;
}

bool AnimationSystem::SwitchPerspective(Animation::SwitchPerspectiveParams & params)
{
  Animation::PropertyValue value;
  if (GetProperty(Animation::MapPlane, Animation::SwitchPerspective, value))
  {
    params = value.m_valuePerspectiveParams;
    return true;
  }
  return false;
}

bool AnimationSystem::AnimationExists(Animation::TObject object) const
{
  if (!m_animationChain.empty())
  {
    for (auto const & anim : m_animationChain.front())
    {
      if (anim->HasObject(object))
        return true;
    }
  }
  for (auto const & prop : m_propertyCache)
  {
    if (prop.first.first == object)
      return true;
  }
  return false;
}

bool AnimationSystem::HasAnimations() const
{
  return !m_animationChain.empty();
}

AnimationSystem & AnimationSystem::Instance()
{
  static AnimationSystem animSystem;
  return animSystem;
}

void AnimationSystem::CombineAnimation(drape_ptr<Animation> animation)
{
  for (auto & lst : m_animationChain)
  {
    bool couldBeBlended = animation->CouldBeBlended();
    for (auto it = lst.begin(); it != lst.end();)
    {
      auto & anim = *it;
      if (anim->GetInterruptedOnCombine())
      {
        anim->Interrupt();
        SaveAnimationResult(*anim);
        it = lst.erase(it);
      }
      else if (!anim->CouldBeBlendedWith(*animation))
      {
        if (!anim->CouldBeInterrupted())
        {
          couldBeBlended = false;
          break;
        }
        anim->Interrupt();
        SaveAnimationResult(*anim);
        it = lst.erase(it);
      }
      else
      {
        ++it;
      }
    }

    if (couldBeBlended)
    {
      animation->OnStart();
      lst.emplace_back(move(animation));
      return;
    }
    else if (m_animationChain.size() > 1 && animation->CouldBeInterrupted())
    {
      return;
    }
  }
  PushAnimation(move(animation));
}

void AnimationSystem::PushAnimation(drape_ptr<Animation> animation)
{
  if (m_animationChain.empty())
    animation->OnStart();

  TAnimationList list;
  list.emplace_back(move(animation));

  m_animationChain.emplace_back(move(list));
}

void AnimationSystem::FinishAnimations(function<bool(drape_ptr<Animation> const &)> const & predicate,
                                       bool rewind, bool finishAll)
{
  if (m_animationChain.empty())
    return;

  TAnimationList & frontList = m_animationChain.front();
  for (auto it = frontList.begin(); it != frontList.end();)
  {
    auto & anim = *it;
    if (predicate(anim))
    {
      if (rewind)
        anim->Finish();
      SaveAnimationResult(*anim);
      it = frontList.erase(it);
    }
    else
    {
      ++it;
    }
  }

  if (finishAll)
  {
    for (auto & lst : m_animationChain)
    {
      for (auto it = lst.begin(); it != lst.end();)
      {
        if (predicate(*it))
          it = lst.erase(it);
        else
          ++it;
      }
    }
  }

  if (frontList.empty())
    StartNextAnimations();
}

void AnimationSystem::FinishAnimations(Animation::Type type, bool rewind, bool finishAll)
{
  FinishAnimations([&type](drape_ptr<Animation> const & anim) { return anim->GetType() == type; },
                   rewind, finishAll);
}

void AnimationSystem::FinishObjectAnimations(Animation::TObject object, bool rewind, bool finishAll)
{
  FinishAnimations([&object](drape_ptr<Animation> const & anim) { return anim->HasObject(object); },
                   rewind, finishAll);
}

void AnimationSystem::Advance(double elapsedSeconds)
{
  if (m_animationChain.empty())
    return;

  TAnimationList & frontList = m_animationChain.front();
  for (auto it = frontList.begin(); it != frontList.end();)
  {
    auto & anim = *it;
    anim->Advance(elapsedSeconds);
    if (anim->IsFinished())
    {
      anim->OnFinish();
      SaveAnimationResult(*anim);
      it = frontList.erase(it);
    }
    else
    {
      ++it;
    }
  }
  if (frontList.empty())
    StartNextAnimations();
}

bool AnimationSystem::GetProperty(Animation::TObject object, Animation::TProperty property,
                                  Animation::PropertyValue & value) const
{
  if (!m_animationChain.empty())
  {
    PropertyBlender blender;
    for (auto const & anim : m_animationChain.front())
    {
      if (anim->HasProperty(object, property))
      {
        Animation::PropertyValue val;
        if (anim->GetProperty(object, property, val))
          blender.Blend(val);
      }
    }
    if (!blender.IsEmpty())
    {
      value = blender.Finish();
      return true;
    }
  }

  auto it = m_propertyCache.find(make_pair(object, property));
  if (it != m_propertyCache.end())
  {
    value = it->second;
    m_propertyCache.erase(it);
    return true;
  }
  return false;
}

void AnimationSystem::SaveAnimationResult(Animation const & animation)
{
  for (auto const & object : animation.GetObjects())
  {
    for (auto const & property : animation.GetProperties(object))
    {
      Animation::PropertyValue value;
      if (animation.GetProperty(object, property, value))
        m_propertyCache[make_pair(object, property)] = value;
    }
  }
}

void AnimationSystem::StartNextAnimations()
{
  if (m_animationChain.empty())
    return;

  m_animationChain.pop_front();
  if (!m_animationChain.empty())
  {
    for (auto & anim : m_animationChain.front())
    {
      //TODO (in future): use propertyCache to load start values to the next animations
      anim->OnStart();
    }
  }
}

} // namespace df
