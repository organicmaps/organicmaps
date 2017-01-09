#pragma once

#include "animation/animation.hpp"

#include "geometry/screenbase.hpp"

#include "std/cstring.hpp"
#include "std/deque.hpp"
#include "std/list.hpp"
#include "std/noncopyable.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"

//#define DEBUG_ANIMATIONS

namespace df
{

class AnimationSystem : private noncopyable
{
public:
  static AnimationSystem & Instance();

  void UpdateLastScreen(ScreenBase const & currentScreen);
  bool GetScreen(ScreenBase const & currentScreen, ScreenBase & screen);
  void GetTargetScreen(ScreenBase const & currentScreen, ScreenBase & screen);

  bool GetArrowPosition(m2::PointD & position);
  bool GetArrowAngle(double & angle);

  bool AnimationExists(Animation::Object object) const;
  bool HasAnimations() const;

  void CombineAnimation(drape_ptr<Animation> && animation);
  void PushAnimation(drape_ptr<Animation> && animation);

  void FinishAnimations(Animation::Type type, bool rewind, bool finishAll);
  void FinishAnimations(Animation::Type type, string const & customType, bool rewind, bool finishAll);
  void FinishObjectAnimations(Animation::Object object, bool rewind, bool finishAll);

  template<typename T> T const * FindAnimation(Animation::Type type, char const * customType = nullptr) const
  {
    for (auto & pList : m_animationChain)
    {
      auto & lst = *pList;
      for (auto const & anim : lst)
      {
        if ((anim->GetType() == type) &&
            (customType == nullptr || strcmp(anim->GetCustomType().c_str(), customType) == 0))
        {
          ASSERT(dynamic_cast<T const *>(anim.get()) != nullptr, ());
          return static_cast<T const *>(anim.get());
        }
      }
    }
    return nullptr;
  }

  void Advance(double elapsedSeconds);

  ScreenBase const & GetLastScreen() { return m_lastScreen; }
  void SaveAnimationResult(Animation const & animation);

private:  
  AnimationSystem() = default;

  using TGetPropertyFn = function<bool (Animation::Object object, Animation::ObjectProperty property,
                                        Animation::PropertyValue & value)>;
  bool GetScreen(ScreenBase const & currentScreen, TGetPropertyFn const & getPropertyFn,  ScreenBase & screen);

  bool GetProperty(Animation::Object object, Animation::ObjectProperty property,
                   Animation::PropertyValue & value) const;
  bool GetTargetProperty(Animation::Object object, Animation::ObjectProperty property,
                   Animation::PropertyValue & value) const;
  void StartNextAnimations();
  void FinishAnimations(function<bool(shared_ptr<Animation> const &)> const & predicate,
                        bool rewind, bool finishAll);

#ifdef DEBUG_ANIMATIONS
  void Print();
#endif

  using TAnimationList = list<shared_ptr<Animation>>;
  using TAnimationChain = deque<shared_ptr<TAnimationList>>;
  using TPropertyCache = map<pair<Animation::Object, Animation::ObjectProperty>, Animation::PropertyValue>;

  TAnimationChain m_animationChain;
  mutable TPropertyCache m_propertyCache;

  ScreenBase m_lastScreen;
};

}
