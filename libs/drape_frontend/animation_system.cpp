#include "animation_system.hpp"

#include "base/logging.hpp"

#include <vector>

using namespace std::placeholders;

namespace df
{
namespace
{
class PropertyBlender
{
public:
  PropertyBlender() = default;

  void Blend(Animation::PropertyValue const & value)
  {
    if (m_counter != 0)
    {
      // New value type resets current blended value.
      if (m_value.m_type != value.m_type)
      {
        m_value = value;
        m_counter = 1;
        return;
      }

      if (value.m_type == Animation::PropertyValue::Type::ValueD)
        m_value.m_valueD += value.m_valueD;
      else if (value.m_type == Animation::PropertyValue::Type::ValuePointD)
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
    if (m_value.m_type == Animation::PropertyValue::Type::ValueD)
      m_value.m_valueD *= scalar;
    else if (m_value.m_type == Animation::PropertyValue::Type::ValuePointD)
      m_value.m_valuePointD *= scalar;

    return m_value;
  }

  bool IsEmpty() const { return m_counter == 0; }

private:
  Animation::PropertyValue m_value;
  uint32_t m_counter = 0;
};
}  // namespace

void AnimationSystem::UpdateLastScreen(ScreenBase const & currentScreen)
{
  m_lastScreen = currentScreen;
}

bool AnimationSystem::GetScreen(ScreenBase const & currentScreen, ScreenBase & screen)
{
  return GetScreen(currentScreen, std::bind(&AnimationSystem::GetProperty, this, _1, _2, _3), screen);
}

void AnimationSystem::GetTargetScreen(ScreenBase const & currentScreen, ScreenBase & screen)
{
  GetScreen(currentScreen, std::bind(&AnimationSystem::GetTargetProperty, this, _1, _2, _3), screen);
}

bool AnimationSystem::GetScreen(ScreenBase const & currentScreen, TGetPropertyFn const & getPropertyFn,
                                ScreenBase & screen)
{
  ASSERT(getPropertyFn != nullptr, ());

  m_lastScreen = currentScreen;

  double scale = currentScreen.GetScale();
  double angle = currentScreen.GetAngle();
  m2::PointD pos = currentScreen.GlobalRect().GlobalZero();

  Animation::PropertyValue value;
  if (getPropertyFn(Animation::Object::MapPlane, Animation::ObjectProperty::Scale, value))
    scale = value.m_valueD;

  if (getPropertyFn(Animation::Object::MapPlane, Animation::ObjectProperty::Angle, value))
    angle = value.m_valueD;

  if (getPropertyFn(Animation::Object::MapPlane, Animation::ObjectProperty::Position, value))
    pos = value.m_valuePointD;

  screen = currentScreen;
  screen.SetFromParams(pos, angle, scale);

  return true;
}

bool AnimationSystem::GetArrowPosition(m2::PointD & position)
{
  Animation::PropertyValue value;
  if (GetProperty(Animation::Object::MyPositionArrow, Animation::ObjectProperty::Position, value))
  {
    position = value.m_valuePointD;
    return true;
  }
  return false;
}

bool AnimationSystem::GetArrowAngle(double & angle)
{
  Animation::PropertyValue value;
  if (GetProperty(Animation::Object::MyPositionArrow, Animation::ObjectProperty::Angle, value))
  {
    angle = value.m_valueD;
    return true;
  }
  return false;
}

bool AnimationSystem::AnimationExists(Animation::Object object) const
{
  if (!m_animationChain.empty())
  {
    for (auto const & anim : *(m_animationChain.front()))
      if (anim->HasObject(object))
        return true;
  }
  for (auto const & prop : m_propertyCache)
    if (prop.first.first == object)
      return true;
  return false;
}

bool AnimationSystem::HasAnimations() const
{
  return !m_animationChain.empty();
}

bool AnimationSystem::HasMapAnimations() const
{
  if (AnimationExists(Animation::Object::MapPlane))
    return true;

  if (AnimationExists(Animation::Object::Selection))
    return true;

  return false;
}

AnimationSystem & AnimationSystem::Instance()
{
  static AnimationSystem animSystem;
  return animSystem;
}

void AnimationSystem::CombineAnimation(drape_ptr<Animation> && animation)
{
#ifdef DEBUG_ANIMATIONS
  LOG(LINFO, ("Combine animation", animation->GetType()));
#endif
  TAnimationList interruptedAnimations;
  bool startImmediately = true;
  for (auto & pList : m_animationChain)
  {
    auto & lst = *pList;
    bool couldBeBlended = animation->CouldBeBlended();

    for (auto it = lst.begin(); it != lst.end();)
    {
      auto & anim = *it;
      if (anim->GetInterruptedOnCombine() && anim->HasSameObjects(*animation))
      {
#ifdef DEBUG_ANIMATIONS
        LOG(LINFO, ("Interrupted on combine", anim->GetType()));
#endif
        interruptedAnimations.splice(interruptedAnimations.end(), lst, it++);
      }
      else if (!anim->CouldBeBlendedWith(*animation))
      {
        if (!anim->CouldBeInterrupted())
        {
          couldBeBlended = false;
          break;
        }
#ifdef DEBUG_ANIMATIONS
        LOG(LINFO, ("Couldn't be blended, interrupted", anim->GetType()));
#endif
        interruptedAnimations.splice(interruptedAnimations.end(), lst, it++);
      }
      else
      {
        ++it;
      }
    }

    for (auto & anim : interruptedAnimations)
    {
      anim->Interrupt();
      SaveAnimationResult(*anim);
    }

    if (couldBeBlended)
    {
#ifdef DEBUG_ANIMATIONS
      LOG(LINFO, ("Animation blended"));
#endif
      lst.emplace_back(std::move(animation));
      if (startImmediately)
        lst.back()->OnStart();
#ifdef DEBUG_ANIMATIONS
      Print();
#endif
      return;
    }
    else if (m_animationChain.size() > 1 && animation->CouldBeInterrupted())
    {
#ifdef DEBUG_ANIMATIONS
      LOG(LINFO, ("Animation rejected"));
      Print();
#endif
      return;
    }

    startImmediately = false;
  }

  PushAnimation(std::move(animation));
}

void AnimationSystem::PushAnimation(drape_ptr<Animation> && animation)
{
#ifdef DEBUG_ANIMATIONS
  LOG(LINFO, ("Push animation", animation->GetType()));
#endif

  auto pList = std::make_shared<TAnimationList>();
  pList->emplace_back(std::move(animation));

  bool startImmediately = m_animationChain.empty();
  m_animationChain.push_back(pList);
  if (startImmediately)
    pList->front()->OnStart();

#ifdef DEBUG_ANIMATIONS
  Print();
#endif
}

void AnimationSystem::FinishAnimations(std::function<bool(std::shared_ptr<Animation> const &)> const & predicate,
                                       bool rewind, bool finishAll)
{
  if (m_animationChain.empty())
    return;

#ifdef DEBUG_ANIMATIONS
  bool changed = false;
#endif

  TAnimationList & frontList = *(m_animationChain.front());
  TAnimationList finishAnimations;
  for (auto it = frontList.begin(); it != frontList.end();)
  {
    auto & anim = *it;
    if (predicate(anim))
    {
#ifdef DEBUG_ANIMATIONS
      LOG(LINFO,
          ("Finish animation", anim->GetType(), ", rewind:", rewind, ", couldBeRewinded:", anim->CouldBeRewinded()));
      changed = true;
#endif
      finishAnimations.splice(finishAnimations.end(), frontList, it++);
    }
    else
    {
      ++it;
    }
  }

  if (finishAll)
  {
    for (auto & pList : m_animationChain)
    {
      auto & lst = *pList;
      for (auto it = lst.begin(); it != lst.end();)
      {
        if (predicate(*it))
        {
#ifdef DEBUG_ANIMATIONS
          LOG(LINFO, ("Finish animation", (*it)->GetType(), ", rewind:", rewind,
                      ", couldBeRewinded:", (*it)->CouldBeRewinded()));
          changed = true;
#endif
          it = lst.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }

  for (auto & anim : finishAnimations)
  {
    if (rewind && anim->CouldBeRewinded())
      anim->Finish();
    SaveAnimationResult(*anim);
  }

#ifdef DEBUG_ANIMATIONS
  if (changed)
    Print();
#endif

  if (frontList.empty())
    StartNextAnimations();
}

void AnimationSystem::FinishAnimations(Animation::Type type, bool rewind, bool finishAll)
{
  FinishAnimations([&type](std::shared_ptr<Animation> const & anim) { return anim->GetType() == type; }, rewind,
                   finishAll);
}

void AnimationSystem::FinishAnimations(Animation::Type type, std::string const & customType, bool rewind,
                                       bool finishAll)
{
  FinishAnimations([&type, &customType](std::shared_ptr<Animation> const & anim)
  { return anim->GetType() == type && anim->GetCustomType() == customType; }, rewind, finishAll);
}

void AnimationSystem::FinishObjectAnimations(Animation::Object object, bool rewind, bool finishAll)
{
  FinishAnimations([&object](std::shared_ptr<Animation> const & anim) { return anim->HasObject(object); }, rewind,
                   finishAll);
}

void AnimationSystem::Advance(double elapsedSeconds)
{
  if (!HasAnimations())
    return;

  TAnimationList finishedAnimations;
  TAnimationList & frontList = *(m_animationChain.front());
  for (auto it = frontList.begin(); it != frontList.end();)
  {
    auto & anim = *it;
    anim->Advance(elapsedSeconds);
    if (anim->IsFinished())
      finishedAnimations.splice(finishedAnimations.end(), frontList, it++);
    else
      ++it;
  }
  for (auto & anim : finishedAnimations)
  {
    anim->OnFinish();
    SaveAnimationResult(*anim);
  }
  if (frontList.empty())
    StartNextAnimations();
}

#ifdef DEBUG_ANIMATIONS
void AnimationSystem::Print()
{
  LOG(LINFO, ("-----------------------Animation chain begin-----------------------"));
  for (size_t i = 0, sz = m_animationChain.size(); i < sz; ++i)
  {
    auto & lst = *m_animationChain[i];
    if (i > 0)
      LOG(LINFO, ("- - - - - - - - - - - - Next parallel block - - - - - - - - - - - -"));
    for (auto it = lst.begin(); it != lst.end(); ++it)
    {
      auto & anim = *it;
      LOG(LINFO, ("Type:", anim->GetType()));
    }
  }
  LOG(LINFO, ("========================Animation chain end========================"));
}
#endif

bool AnimationSystem::GetProperty(Animation::Object object, Animation::ObjectProperty property,
                                  Animation::PropertyValue & value) const
{
  if (!m_animationChain.empty())
  {
    PropertyBlender blender;
    for (auto const & anim : *(m_animationChain.front()))
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

  auto it = m_propertyCache.find(std::make_pair(object, property));
  if (it != m_propertyCache.end())
  {
    value = it->second;
    m_propertyCache.erase(it);
    return true;
  }
  return false;
}

bool AnimationSystem::GetTargetProperty(Animation::Object object, Animation::ObjectProperty property,
                                        Animation::PropertyValue & value) const
{
  if (!m_animationChain.empty())
  {
    PropertyBlender blender;
    for (auto const & anim : *(m_animationChain.front()))
    {
      if (anim->HasTargetProperty(object, property))
      {
        Animation::PropertyValue val;
        if (anim->GetTargetProperty(object, property, val))
          blender.Blend(val);
      }
    }
    if (!blender.IsEmpty())
    {
      value = blender.Finish();
      return true;
    }
  }

  auto it = m_propertyCache.find(std::make_pair(object, property));
  if (it != m_propertyCache.end())
  {
    value = it->second;
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
        m_propertyCache[std::make_pair(object, property)] = value;
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
    std::vector<std::weak_ptr<Animation>> startedAnimations;
    startedAnimations.reserve(m_animationChain.front()->size());
    for (auto & anim : *(m_animationChain.front()))
      startedAnimations.push_back(anim);

    for (auto & weak_anim : startedAnimations)
    {
      std::shared_ptr<Animation> anim = weak_anim.lock();
      if (anim != nullptr)
      {
        anim->Init(m_lastScreen, m_propertyCache);
        anim->OnStart();
      }
    }
  }
}
}  // namespace df
