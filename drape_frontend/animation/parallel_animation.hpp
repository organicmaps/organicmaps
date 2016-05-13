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

  TAnimObjects const & GetObjects() const override
  {
    return m_objects;
  }

  bool HasObject(TObject object) const override
  {
    return m_objects.find(object) != m_objects.end();
  }

  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void AddAnimation(drape_ptr<Animation> animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

private:
  list<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<TObject, TObjectProperties> m_properties;
};

} // namespace df
