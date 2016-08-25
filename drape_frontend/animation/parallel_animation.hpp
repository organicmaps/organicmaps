#pragma once

#include "animation.hpp"

#include "drape/pointers.hpp"

namespace df
{

class ParallelAnimation : public Animation
{
public:
  ParallelAnimation();

  Animation::Type GetType() const override { return Animation::Parallel; }

  TAnimObjects const & GetObjects() const override;
  bool HasObject(TObject object) const override;

  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;
  bool HasTargetProperty(TObject object, TProperty property) const override;

  string GetCustomType() const override;
  void SetCustomType(string const & type);

  void AddAnimation(drape_ptr<Animation> && animation);

  void OnStart() override;
  void OnFinish() override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override;
  bool GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const override;

  template<typename T> T const * FindAnimation(Animation::Type type, char const * customType = nullptr) const
  {
    for (auto const & anim : m_animations)
    {
      if ((anim->GetType() == type) &&
          (customType == nullptr || anim->GetCustomType() == customType))
      {
        ASSERT(dynamic_cast<T const *>(anim.get()) != nullptr, ());
        return static_cast<T const *>(anim.get());
      }
    }
    return nullptr;
  }

private:
  void ObtainObjectProperties();

  list<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<TObject, TObjectProperties> m_properties;

  string m_customType;
};

} // namespace df
