#include "animation_system.h"
#include "animation/interpolations.hpp"

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

void Interpolator::SetMaxDuration(double maxDuration)
{
  m_duration = min(m_duration, maxDuration);
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
  double const kMinMoveDuration = 0.2;
  double const kMinSpeedScalar = 0.2;
  double const kMaxSpeedScalar = 7.0;
  double const kEps = 1e-5;

  m2::RectD const & dispPxRect = convertor.PixelRect();
  double const pixelLength = convertor.GtoP(endPosition).Length(convertor.GtoP(startPosition));
  if (pixelLength < kEps)
    return 0.0;

  double const minSize = min(dispPxRect.SizeX(), dispPxRect.SizeY());
  if (pixelLength < kMinSpeedScalar * minSize)
    return kMinMoveDuration;

  double const pixelSpeed = kMaxSpeedScalar * minSize;
  return CalcAnimSpeedDuration(pixelLength, pixelSpeed);
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

void PositionInterpolator::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  InterpolatePoint(m_startPosition, m_endPosition, GetT());
}

// static
double AngleInterpolator::GetRotateDuration(double startAngle, double endAngle)
{
  return 0.5 * fabs(endAngle - startAngle) / math::pi4;
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

void AngleInterpolator::Advance(double elapsedSeconds)
{
  TBase::Advance(elapsedSeconds);
  m_angle = InterpolateDouble(m_startAngle, m_endAngle, GetT());
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

FollowAnimation::FollowAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                                 double startAngle, double endAngle,
                                 double startScale, double endScale, ScreenBase const & convertor)
  : Animation(false, false)
{
  SetMove(startPos, endPos, convertor);
  SetRotate(startAngle, endAngle);
  SetScale(startScale, endScale);
}

FollowAnimation::FollowAnimation()
  : Animation(false, false)
{
}

void FollowAnimation::SetMove(m2::PointD const & startPos, m2::PointD const & endPos,
                              ScreenBase const & convertor)
{
  if (startPos != endPos)
    m_positionInterpolator = make_unique_dp<PositionInterpolator>(startPos, endPos, convertor);
}

void FollowAnimation::SetRotate(double startAngle, double endAngle)
{
  if (startAngle != endAngle)
    m_angleInterpolator = make_unique_dp<AngleInterpolator>(startAngle, endAngle);
}

void FollowAnimation::SetScale(double startScale, double endScale)
{
  if (startScale != endScale)
    m_scaleInterpolator = make_unique_dp<ScaleInterpolator>(startScale, endScale);
}

void FollowAnimation::Advance(double elapsedSeconds)
{
  if (m_angleInterpolator != nullptr)
    m_angleInterpolator->Advance(elapsedSeconds);
  if (m_scaleInterpolator != nullptr)
    m_scaleInterpolator->Advance(elapsedSeconds);
  if (m_positionInterpolator != nullptr)
    m_positionInterpolator->Advance(elapsedSeconds);
}

void FollowAnimation::SetMaxDuration(double maxDuration)
{
  if (m_angleInterpolator != nullptr)
    m_angleInterpolator->SetMaxDuration(maxDuration);
  if (m_scaleInterpolator != nullptr)
    m_scaleInterpolator->SetMaxDuration(maxDuration);
  if (m_positionInterpolator != nullptr)
    m_positionInterpolator->SetMaxDuration(maxDuration);
}

double FollowAnimation::GetDuration() const
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

bool FollowAnimation::IsFinished() const
{
  return ((m_angleInterpolator == nullptr || m_angleInterpolator->IsFinished())
      && (m_scaleInterpolator == nullptr || m_scaleInterpolator->IsFinished())
      && (m_positionInterpolator == nullptr || m_positionInterpolator->IsFinished()));
}

double FollowAnimation::GetScale(uint32_t object) const
{
  ASSERT(object & GetObjects(), ());
  ASSERT(m_scaleInterpolator != nullptr, ());

  if (m_scaleInterpolator != nullptr)
    return m_scaleInterpolator->GetScale();

  return 0.0;
}

double FollowAnimation::GetAngle(uint32_t object) const
{
  ASSERT(object & GetObjects(), ());
  ASSERT(m_angleInterpolator != nullptr, ());

  if (m_angleInterpolator != nullptr)
    return m_angleInterpolator->GetAngle();

  return 0.0;
}

m2::PointD FollowAnimation::GetPosition(uint32_t object) const
{
  ASSERT(object & GetObjects(), ());
  ASSERT(m_positionInterpolator != nullptr, ());

  if (m_positionInterpolator != nullptr)
    return m_positionInterpolator->GetPosition();

  return m2::PointD();
}

uint32_t ParallelAnimation::GetMask(uint32_t object) const
{
  int mask = 0;
  for (auto const & anim : m_animations)
    mask |= anim->GetMask(object);
  return mask;
}

uint32_t ParallelAnimation::GetObjects() const
{
  int objects = 0;
  for (auto const & anim : m_animations)
    objects |= anim->GetObjects();
  return objects;
}

void ParallelAnimation::AddAnimation(ref_ptr<Animation> animation)
{
  m_animations.push_back(animation);
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

uint32_t SequenceAnimation::GetMask(uint32_t object) const
{
  int mask = 0;
  if (!m_animations.empty())
    mask = m_animations.front()->GetMask(object);
  return mask;
}

uint32_t SequenceAnimation::GetObjects() const
{
  int objects = 0;
  if (!m_animations.empty())
    objects = m_animations.front()->GetObjects();
  return objects;
}

void SequenceAnimation::AddAnimation(ref_ptr<Animation> animation)
{
  m_animations.push_back(animation);
}

void SequenceAnimation::OnStart()
{
  if (m_animations.empty())
    return;
  m_animations.front()->OnStart();
}

void SequenceAnimation::OnFinish()
{

}

void SequenceAnimation::Advance(double elapsedSeconds)
{
  if (m_animations.empty())
    return;
  m_animations.front()->Advance(elapsedSeconds);
  if (m_animations.front()->IsFinished())
  {
    m_animations.front()->OnFinish();
    m_animations.pop_front();
  }
}

m2::AnyRectD AnimationSystem::GetRect(ScreenBase const & currentScreen)
{
  double scale = currentScreen.GetScale();
  double angle = currentScreen.GetAngle();
  m2::PointD pos = currentScreen.GlobalRect().GlobalZero();
  for (auto const & anim : m_animations)
  {
    if (anim->GetObjects() & kPlaneAnimObjBit)
    {
      int mask = anim->GetMask(kPlaneAnimObjBit);
      if (mask & kScaleAnimationBit)
        scale = anim->GetScale(kPlaneAnimObjBit);
      if (mask & kMoveAnimationBit)
        pos = anim->GetPosition(kPlaneAnimObjBit);
      if (mask & kRotateAnimationBit)
        angle = anim->GetAngle(kPlaneAnimObjBit);
    }
  }
  m2::RectD rect = currentScreen.PixelRect();
  rect.Offset(-rect.Center());
  rect.Scale(scale);
  return m2::AnyRectD(pos, angle, rect);
}

bool AnimationSystem::AnimationExists(Animation::Object object)
{
  for (auto const & anim : m_animations)
  {
    if (anim->GetObjects() & (1 << object))
      return true;
  }
  return false;
}

AnimationSystem & AnimationSystem::Instance()
{
  static AnimationSystem animSystem;
  return animSystem;
}

void AnimationSystem::AddAnimation(drape_ptr<Animation> && animation)
{
  animation->OnStart();
  m_animations.insert(move(animation));
}

void AnimationSystem::Advance(double elapsedSeconds)
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

} // namespace df
