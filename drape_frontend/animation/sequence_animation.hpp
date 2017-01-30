#pragma once

#include "animation.hpp"

#include "drape/pointers.hpp"

#include "std/deque.hpp"

namespace df
{

class SequenceAnimation : public Animation
{
public:
  SequenceAnimation();

  void Init(ScreenBase const & screen, TPropertyCache const & properties) override;

  Animation::Type GetType() const override { return Animation::Type::Sequence; }
  TAnimObjects const & GetObjects() const override;
  bool HasObject(Object object) const override;
  TObjectProperties const & GetProperties(Object object) const override;
  bool HasProperty(Object object, ObjectProperty property) const override;
  bool HasTargetProperty(Object object, ObjectProperty property) const override;

  string GetCustomType() const override;
  void SetCustomType(string const & type);

  void SetMaxDuration(double maxDuration) override;
  void SetMinDuration(double minDuration) override;
  double GetDuration() const override;
  double GetMaxDuration() const override;
  double GetMinDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(Object object, ObjectProperty property, PropertyValue &value) const override;
  bool GetTargetProperty(Object object, ObjectProperty property, PropertyValue &value) const override;

  void AddAnimation(drape_ptr<Animation> && animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

private:
  void ObtainObjectProperties();

  deque<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<Object, TObjectProperties> m_properties;

  string m_customType;
};

} // namespace df

