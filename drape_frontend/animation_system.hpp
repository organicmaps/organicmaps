#pragma once

#include "animation/animation.hpp"

#include "geometry/screenbase.hpp"

#include "std/deque.hpp"
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

  bool GetRect(ScreenBase const & currentScreen, m2::AnyRectD & rect);
  void GetTargetRect(ScreenBase const & currentScreen, m2::AnyRectD & rect);

  bool SwitchPerspective(Animation::SwitchPerspectiveParams & params);
  bool GetPerspectiveAngle(double & angle);

  bool GetArrowPosition(m2::PointD & position);
  bool GetArrowAngle(double & angle);

  bool AnimationExists(Animation::TObject object) const;
  bool HasAnimations() const;

  void CombineAnimation(drape_ptr<Animation> animation);
  void PushAnimation(drape_ptr<Animation> animation);

  void FinishAnimations(Animation::Type type, bool rewind, bool finishAll);
  void FinishAnimations(Animation::Type type, string const & customType, bool rewind, bool finishAll);
  void FinishObjectAnimations(Animation::TObject object, bool rewind, bool finishAll);

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

  using TGetPropertyFn = function<bool (Animation::TObject object, Animation::TProperty property,
                                        Animation::PropertyValue & value)>;
  bool GetRect(ScreenBase const & currentScreen, TGetPropertyFn const & getPropertyFn,  m2::AnyRectD & rect);

  bool GetProperty(Animation::TObject object, Animation::TProperty property,
                   Animation::PropertyValue & value) const;
  bool GetTargetProperty(Animation::TObject object, Animation::TProperty property,
                   Animation::PropertyValue & value) const;
  void StartNextAnimations();
  void FinishAnimations(function<bool(shared_ptr<Animation> const &)> const & predicate,
                        bool rewind, bool finishAll);

#ifdef DEBUG_ANIMATIONS
  void Print();
#endif

  using TAnimationList = list<shared_ptr<Animation>>;
  using TAnimationChain = deque<shared_ptr<TAnimationList>>;
  using TPropertyCache = map<pair<Animation::TObject, Animation::TProperty>, Animation::PropertyValue>;

  TAnimationChain m_animationChain;
  mutable TPropertyCache m_propertyCache;

  ScreenBase m_lastScreen;
};

}
