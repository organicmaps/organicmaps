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
  Animation::Type GetType() const override { return Animation::Sequence; }
  TAnimObjects const & GetObjects() const override;
  bool HasObject(TObject object) const override;
  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;
  bool HasTargetProperty(TObject object, TProperty property) const override;

  string GetCustomType() const override;
  void SetCustomType(string const & type);

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(TObject object, TProperty property, PropertyValue &value) const override;
  bool GetTargetProperty(TObject object, TProperty property, PropertyValue &value) const override;

  void AddAnimation(drape_ptr<Animation> animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

private:
  void ObtainObjectProperties();

  deque<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<TObject, TObjectProperties> m_properties;

  string m_customType;
};

} // namespace df

