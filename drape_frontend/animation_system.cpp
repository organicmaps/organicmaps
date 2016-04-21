#include "animation_system.h"
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

}

bool Animation::CouldBeMixedWith(TObject object, TObjectProperties const & properties)
{
  if (!m_couldBeMixed)
    return false;
  ASSERT(HasObject(object), ());

  TObjectProperties const & selfProperties = GetProperties(object);
  TObjectProperties intersection;
  set_intersection(selfProperties.begin(), selfProperties.end(),
                   properties.begin(), properties.end(),
                   inserter(intersection, intersection.end()));
  return intersection.empty();
}

bool Animation::CouldBeMixedWith(Animation const & animation)
{
  if (!m_couldBeMixed || animation.m_couldBeMixed)
    return false;

  for (auto const & object : animation.GetObjects())
  {
    if (!HasObject(object))
      continue;
    if (!CouldBeMixedWith(object, animation.GetProperties(object)))
      return false;
  }
  return true;
}

Interpolator::Interpolator(double duration, double delay)
  : m_elapsedTime(0.0)
  , m_duration(duration)
  , m_delay(delay)
{
  ASSERT(m_duration >= 0.0, ());
}

Interpolator::~Interpolator()
{
}

bool Interpolator::IsFinished() const
{
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

//static
double PositionInterpolator::GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor)
{
  return GetPixelMoveDuration(convertor.GtoP(startPosition), convertor.GtoP(endPosition), convertor.PixelRectIn3d());
}

double PositionInterpolator::GetPixelMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & pixelRect)
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

PositionInterpolator::PositionInterpolator(double duration, double delay, m2::PointD const & startPosition, m2::PointD const & endPosition)
  : Interpolator(duration, delay)
  , m_startPosition(startPosition)
  , m_endPosition(endPosition)
  , m_position(startPosition)
{

}

PositionInterpolator::PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor)
  : PositionInterpolator(0.0 /* delay */, startPosition, endPosition, convertor)
{
}

PositionInterpolator::PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor)
  : Interpolator(PositionInterpolator::GetMoveDuration(startPosition, endPosition, convertor), delay)
  , m_startPosition(startPosition)
  , m_endPosition(endPosition)
  , m_position(startPosition)
{

}

PositionInterpolator::PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & pixelRect)
  : PositionInterpolator(0.0 /* delay */, startPosition, endPosition, pixelRect)
{

}

PositionInterpolator::PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & pixelRect)
  : Interpolator(PositionInterpolator::GetPixelMoveDuration(startPosition, endPosition, pixelRect), delay)
  , m_startPosition(startPosition)
  , m_endPosition(endPosition)
  , m_position(startPosition)
{

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

// static
double AngleInterpolator::GetRotateDuration(double startAngle, double endAngle)
{
  double const kRotateDurationScalar = 0.75;

  return kRotateDurationScalar * fabs(ang::GetShortestDistance(startAngle, endAngle)) / math::pi;
}

AngleInterpolator::AngleInterpolator(double startAngle, double endAngle)
  : AngleInterpolator(0.0 /* delay */, startAngle, endAngle)
{
}

AngleInterpolator::AngleInterpolator(double delay, double startAngle, double endAngle)
  : Interpolator(AngleInterpolator::GetRotateDuration(startAngle, endAngle), delay)
  , m_startAngle(startAngle)
  , m_endAngle(endAngle)
  , m_angle(startAngle)
{
}

AngleInterpolator::AngleInterpolator(double delay, double duration, double startAngle, double endAngle)
  : Interpolator(duration, delay)
  , m_startAngle(startAngle)
  , m_endAngle(endAngle)
  , m_angle(startAngle)
{
}

void AngleInterpolator::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_angle = InterpolateDouble(m_startAngle, m_endAngle, GetT());
}

void AngleInterpolator::Finish()
{
  TBase::Finish();
  m_angle = m_endAngle;
}

// static
double ScaleInterpolator::GetScaleDuration(double startScale, double endScale)
{
  // Resize 2.0 times should be done for 0.3 seconds.
  double constexpr kPixelSpeed = 2.0 / 0.3;

  if (startScale > endScale)
    swap(startScale, endScale);

  return CalcAnimSpeedDuration(endScale / startScale, kPixelSpeed);
}

ScaleInterpolator::ScaleInterpolator(double startScale, double endScale)
  : ScaleInterpolator(0.0 /* delay */, startScale, endScale)
{
}

ScaleInterpolator::ScaleInterpolator(double delay, double startScale, double endScale)
  : Interpolator(ScaleInterpolator::GetScaleDuration(startScale, endScale), delay)
  , m_startScale(startScale)
  , m_endScale(endScale)
  , m_scale(startScale)
{
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

MapLinearAnimation::MapLinearAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                                 double startAngle, double endAngle,
                                 double startScale, double endScale, ScreenBase const & convertor)
  : Animation(true /* couldBeInterrupted */, false /* couldBeMixed */)
{
  m_objects.insert(Animation::MapPlane);
  SetMove(startPos, endPos, convertor);
  SetRotate(startAngle, endAngle);
  SetScale(startScale, endScale);
}

MapLinearAnimation::MapLinearAnimation()
  : Animation(true /* couldBeInterrupted */, false /* couldBeMixed */)
{
  m_objects.insert(Animation::MapPlane);
}

void MapLinearAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos,
                              ScreenBase const & convertor)
{
  if (startPos != endPos)
  {
    m_positionInterpolator = make_unique_dp<PositionInterpolator>(startPos, endPos, convertor);
    m_properties.insert(Animation::Position);
  }
}

void MapLinearAnimation::SetRotate(double startAngle, double endAngle)
{
  if (startAngle != endAngle)
  {
    m_angleInterpolator = make_unique_dp<AngleInterpolator>(startAngle, endAngle);
    m_properties.insert(Animation::Angle);
  }
}

void MapLinearAnimation::SetScale(double startScale, double endScale)
{
  if (startScale != endScale)
  {
    m_scaleInterpolator = make_unique_dp<ScaleInterpolator>(startScale, endScale);
    m_properties.insert(Animation::Scale);
  }
}

Animation::TObjectProperties const & MapLinearAnimation::GetProperties(TObject object) const
{
  ASSERT(object == Animation::MapPlane, ());
  return m_properties;
}

bool MapLinearAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void MapLinearAnimation::Advance(double elapsedSeconds)
{
  if (m_angleInterpolator != nullptr)
    m_angleInterpolator->Advance(elapsedSeconds);
  if (m_scaleInterpolator != nullptr)
    m_scaleInterpolator->Advance(elapsedSeconds);
  if (m_positionInterpolator != nullptr)
    m_positionInterpolator->Advance(elapsedSeconds);
}

void MapLinearAnimation::Finish()
{
  if (m_angleInterpolator != nullptr)
    m_angleInterpolator->Finish();
  if (m_scaleInterpolator != nullptr)
    m_scaleInterpolator->Finish();
  if (m_positionInterpolator != nullptr)
    m_positionInterpolator->Finish();
  Animation::Finish();
}

void MapLinearAnimation::SetMaxDuration(double maxDuration)
{
  if (m_angleInterpolator != nullptr)
    m_angleInterpolator->SetMaxDuration(maxDuration);
  if (m_scaleInterpolator != nullptr)
    m_scaleInterpolator->SetMaxDuration(maxDuration);
  if (m_positionInterpolator != nullptr)
    m_positionInterpolator->SetMaxDuration(maxDuration);
}

double MapLinearAnimation::GetDuration() const
{
  double duration = 0;
  if (m_angleInterpolator != nullptr)
    duration = m_angleInterpolator->GetDuration();
  if (m_scaleInterpolator != nullptr)
    duration = max(duration, m_scaleInterpolator->GetDuration());
  if (m_positionInterpolator != nullptr)
    duration = max(duration, m_positionInterpolator->GetDuration());
  return duration;
}

bool MapLinearAnimation::IsFinished() const
{
  return ((m_angleInterpolator == nullptr || m_angleInterpolator->IsFinished())
      && (m_scaleInterpolator == nullptr || m_scaleInterpolator->IsFinished())
      && (m_positionInterpolator == nullptr || m_positionInterpolator->IsFinished()));
}

bool MapLinearAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT(object == Animation::MapPlane, ());

  switch (property)
  {
  case Animation::Position:
    ASSERT(m_positionInterpolator != nullptr, ());
    if (m_positionInterpolator != nullptr)
    {
      value = m_positionInterpolator->GetPosition();
      return true;
    }
    return false;
  case Animation::Scale:
    ASSERT(m_scaleInterpolator != nullptr, ());
    if (m_scaleInterpolator != nullptr)
    {
      value = m_scaleInterpolator->GetScale();
      return true;
    }
    return false;
  case Animation::Angle:
    ASSERT(m_angleInterpolator != nullptr, ());
    if (m_angleInterpolator != nullptr)
    {
      value = m_angleInterpolator->GetAngle();
      return true;
    }
    return false;
  default:
    ASSERT(!"Wrong property", ());
  }

  return false;
}

MapScaleAnimation::MapScaleAnimation(double startScale, double endScale,
                               m2::PointD const & globalPosition, m2::PointD const & offset)
  : Animation(true /* couldBeInterrupted */, false /* couldBeMixed */)
  , m_pixelOffset(offset)
  , m_globalPosition(globalPosition)
{
  m_scaleInterpolator = make_unique_dp<ScaleInterpolator>(startScale, endScale);
  m_objects.insert(Animation::MapPlane);
  m_properties.insert(Animation::Scale);
  m_properties.insert(Animation::Position);
}

Animation::TObjectProperties const & MapScaleAnimation::GetProperties(TObject object) const
{
  ASSERT(object == Animation::MapPlane, ());
  return m_properties;
}

bool MapScaleAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void MapScaleAnimation::Advance(double elapsedSeconds)
{
  m_scaleInterpolator->Advance(elapsedSeconds);
}

void MapScaleAnimation::Finish()
{
  m_scaleInterpolator->Finish();
  Animation::Finish();
}

void MapScaleAnimation::SetMaxDuration(double maxDuration)
{
  m_scaleInterpolator->SetMaxDuration(maxDuration);
}

double MapScaleAnimation::GetDuration() const
{
  return m_scaleInterpolator->GetDuration();
}

bool MapScaleAnimation::IsFinished() const
{
  return m_scaleInterpolator->IsFinished();
}

bool MapScaleAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  if (property == Animation::Position)
  {
    ScreenBase screen = AnimationSystem::Instance().GetLastScreen();
    screen.SetScale(m_scaleInterpolator->GetScale());
    value = screen.PtoG(screen.GtoP(m_globalPosition) + m_pixelOffset);
    return true;
  }
  if (property == Animation::Scale)
  {
    value = m_scaleInterpolator->GetScale();
    return true;
  }
  ASSERT(!"Wrong property", ());
  return false;
}

MapFollowAnimation::MapFollowAnimation(m2::PointD const & globalPosition,
                                       double startScale, double endScale,
                                       double startAngle, double endAngle,
                                       m2::PointD const & startPixelPosition, m2::PointD const & endPixelPosition,
                                       m2::RectD const & pixelRect)
  : Animation(true /* couldBeInterrupted */, false /* couldBeMixed */)
  , m_globalPosition(globalPosition)
{
  m_scaleInterpolator = make_unique_dp<ScaleInterpolator>(startScale, endScale);
  m_angleInterpolator = make_unique_dp<AngleInterpolator>(startAngle, endAngle);
  m_pixelPosInterpolator = make_unique_dp<PositionInterpolator>(startPixelPosition, endPixelPosition, pixelRect);
  double const duration = GetDuration();
  m_scaleInterpolator->SetMinDuration(duration);
  m_angleInterpolator->SetMinDuration(duration);
  m_pixelPosInterpolator->SetMinDuration(duration);

  m_objects.insert(Animation::MapPlane);
  m_properties.insert(Animation::Scale);
  m_properties.insert(Animation::Angle);
  m_properties.insert(Animation::Position);
}

Animation::TObjectProperties const & MapFollowAnimation::GetProperties(TObject object) const
{
  ASSERT(object == Animation::MapPlane, ());
  return m_properties;
}

bool MapFollowAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void MapFollowAnimation::Advance(double elapsedSeconds)
{
  m_angleInterpolator->Advance(elapsedSeconds);
  m_scaleInterpolator->Advance(elapsedSeconds);
  m_pixelPosInterpolator->Advance(elapsedSeconds);
}

void MapFollowAnimation::Finish()
{
  m_angleInterpolator->Finish();
  m_scaleInterpolator->Finish();
  m_pixelPosInterpolator->Finish();
  Animation::Finish();
}

void MapFollowAnimation::SetMaxDuration(double maxDuration)
{
  m_angleInterpolator->SetMaxDuration(maxDuration);
  m_scaleInterpolator->SetMaxDuration(maxDuration);
  m_pixelPosInterpolator->SetMaxDuration(maxDuration);
}

double MapFollowAnimation::GetDuration() const
{
  double duration = 0.0;
  if (m_pixelPosInterpolator != nullptr)
    duration = m_angleInterpolator->GetDuration();
  if (m_angleInterpolator != nullptr)
    duration = max(duration, m_angleInterpolator->GetDuration());
  if (m_scaleInterpolator != nullptr)
    duration = max(duration, m_scaleInterpolator->GetDuration());
  return duration;
}

bool MapFollowAnimation::IsFinished() const
{
  return ((m_pixelPosInterpolator == nullptr || m_pixelPosInterpolator->IsFinished())
      && (m_angleInterpolator == nullptr || m_angleInterpolator->IsFinished())
      && (m_scaleInterpolator == nullptr || m_scaleInterpolator->IsFinished()));
}

bool MapFollowAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  if (property == Animation::Position)
  {
    m2::RectD const pixelRect = AnimationSystem::Instance().GetLastScreen().PixelRect();
    m2::PointD const pixelPos = m_pixelPosInterpolator->GetPosition();
    m2::PointD formingVector = (pixelRect.Center() - pixelPos) * m_scaleInterpolator->GetScale();
    formingVector.y = -formingVector.y;
    formingVector.Rotate(m_angleInterpolator->GetAngle());
    value = m_globalPosition + formingVector;
    return true;
  }
  if (property == Animation::Angle)
  {
    value = m_angleInterpolator->GetAngle();
    return true;
  }
  if (property == Animation::Scale)
  {
    value = m_scaleInterpolator->GetScale();
    return true;
  }
  ASSERT(!"Wrong property", ());
  return false;
}

PerspectiveSwitchAnimation::PerspectiveSwitchAnimation(double startAngle, double endAngle, double angleFOV)
  : Animation(false, false)
  , m_startAngle(startAngle)
  , m_endAngle(endAngle)
  , m_angleFOV(angleFOV)
  , m_needPerspectiveSwitch(false)
{
  m_isEnablePerspectiveAnim = m_endAngle > 0.0;
  m_angleInterpolator = make_unique_dp<AngleInterpolator>(GetRotateDuration(startAngle, endAngle), startAngle, endAngle);
  m_objects.insert(Animation::MapPlane);
  m_properties.insert(Animation::AnglePerspective);
  m_properties.insert(Animation::SwitchPerspective);
}

// static
double PerspectiveSwitchAnimation::GetRotateDuration(double startAngle, double endAngle)
{
  return 0.5 * fabs(endAngle - startAngle) / math::pi4;
}

Animation::TObjectProperties const & PerspectiveSwitchAnimation::GetProperties(TObject object) const
{
  ASSERT(object == Animation::MapPlane, ());
  return m_properties;
}

bool PerspectiveSwitchAnimation::HasProperty(TObject object, TProperty property) const
{
  return HasObject(object) && m_properties.find(property) != m_properties.end();
}

void PerspectiveSwitchAnimation::Advance(double elapsedSeconds)
{
  m_angleInterpolator->Advance(elapsedSeconds);
}

void PerspectiveSwitchAnimation::Finish()
{
  m_angleInterpolator->Finish();
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
  m_angleInterpolator->SetMaxDuration(maxDuration);
}

double PerspectiveSwitchAnimation::GetDuration() const
{
  return m_angleInterpolator->GetDuration();
}

bool PerspectiveSwitchAnimation::IsFinished() const
{
  return m_angleInterpolator->IsFinished();
}

bool PerspectiveSwitchAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT(object == Animation::MapPlane, ());

  switch (property)
  {
  case Animation::AnglePerspective:
    value = m_angleInterpolator->GetAngle();
    return true;
  case Animation::SwitchPerspective:
    if (m_needPerspectiveSwitch)
    {
      m_needPerspectiveSwitch = false;
      value = SwitchPerspectiveParams(m_isEnablePerspectiveAnim,
                                      m_startAngle, m_endAngle, m_angleFOV);
      return true;
    }
    return false;
  default:
    ASSERT(!"Wrong property", ());
  }

  return false;
}

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

void ParallelAnimation::AddAnimation(drape_ptr<Animation> && animation)
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
      ++iter;
  }
}

void ParallelAnimation::Finish()
{
  for (auto & anim : m_animations)
    anim->Finish();
  Animation::Finish();
}

SequenceAnimation::SequenceAnimation()
  : Animation(false /* couldBeInterrupted */, false /* couldBeMixed */)
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
  ASSERT(!"Not implemented", ());
}

double SequenceAnimation::GetDuration() const
{
  double duration = 0.0;
  for (auto const & anim : m_animations)
  {
    duration += anim->GetDuration();
  }
  return duration;
}

bool SequenceAnimation::IsFinished() const
{
  return m_animations.empty();
}

bool SequenceAnimation::GetProperty(TObject object, TProperty property, PropertyValue &value) const
{
  ASSERT(!m_animations.empty(), ());
  return m_animations.front()->GetProperty(object, property, value);
}

void SequenceAnimation::AddAnimation(drape_ptr<Animation> && animation)
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
  {
    anim->Finish();
    AnimationSystem::Instance().SaveAnimationResult(*anim);
  }
  m_animations.clear();
  ObtainObjectProperties();
  Animation::Finish();
}

void SequenceAnimation::ObtainObjectProperties()
{
  m_objects.clear();
  m_properties.clear();
  if (!m_animations.empty())
  {
    TAnimObjects const & objects = m_animations.front()->GetObjects();
    m_objects.insert(objects.begin(), objects.end());
    for (auto const & object : objects)
    {
      TObjectProperties const & properties = m_animations.front()->GetProperties(object);
      m_properties[object].insert(properties.begin(), properties.end());
    }
  }
}

AnimationSystem::AnimationSystem()
{

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
  for (auto it = m_propertyCache.begin(); it != m_propertyCache.end(); ++it)
  {
    if (it->first.first == object)
      return true;
  }
  return false;
}

AnimationSystem & AnimationSystem::Instance()
{
  static AnimationSystem animSystem;
  return animSystem;
}

void AnimationSystem::AddAnimation(drape_ptr<Animation> && animation, bool force)
{
  for (auto & lst : m_animationChain)
  {
    bool couldBeMixed = true;
    for (auto it = lst.begin(); it != lst.end();)
    {
      auto & anim = *it;
      if (!anim->CouldBeMixedWith(*animation))
      {
        if (!force || !anim->CouldBeInterrupted())
        {
          couldBeMixed = false;
          break;
        }
        // TODO: do not interrupt anything until it's not clear that we can mix
        anim->Interrupt();
        SaveAnimationResult(*anim);
        it = lst.erase(it);
      }
      else
      {
        ++it;
      }
    }
    if (couldBeMixed)
    {
      animation->OnStart();
      lst.emplace_back(move(animation));
      return;
    }
  }

  PushAnimation(move(animation));
}

void AnimationSystem::PushAnimation(drape_ptr<Animation> && animation)
{
  if (m_animationChain.empty())
    animation->OnStart();

  TAnimationList list;
  list.emplace_back(move(animation));

  m_animationChain.emplace_back(move(list));
}

void AnimationSystem::FinishAnimations(Animation::Type type, bool rewind)
{
  if (m_animationChain.empty())
    return;

  TAnimationList & frontList = m_animationChain.front();
  for (auto it = frontList.begin(); it != frontList.end();)
  {
    auto & anim = *it;
    if (anim->GetType() == type)
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
  if (frontList.empty())
    StartNextAnimations();
}

void AnimationSystem::FinishObjectAnimations(Animation::TObject object, bool rewind)
{
  if (m_animationChain.empty())
    return;

  TAnimationList & frontList = m_animationChain.front();
  for (auto it = frontList.begin(); it != frontList.end();)
  {
    auto & anim = *it;
    if (anim->HasObject(object))
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
  if (frontList.empty())
    StartNextAnimations();
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

bool AnimationSystem::GetProperty(Animation::TObject object, Animation::TProperty property, Animation::PropertyValue & value) const
{
  if (!m_animationChain.empty())
  {
    for (auto const & anim : m_animationChain.front())
    {
      if (anim->HasProperty(object, property))
        return anim->GetProperty(object, property, value);
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
  m_animationChain.pop_front();
  if (m_animationChain.empty())
    return;
  for (auto & anim : m_animationChain.front())
  {
    // TODO: use propertyCache to load start values to the next animations
    anim->OnStart();
  }
}

} // namespace df
