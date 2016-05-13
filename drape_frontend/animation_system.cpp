#include "animation_system.hpp"

#include "base/logging.hpp"

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
