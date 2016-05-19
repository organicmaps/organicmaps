#include "parallel_animation.hpp"

namespace df
{

ParallelAnimation::ParallelAnimation()
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
{}

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

void ParallelAnimation::AddAnimation(drape_ptr<Animation> animation)
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
  for (auto & anim : m_animations)
    anim->OnFinish();
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
    {
      ++iter;
    }
  }
}

void ParallelAnimation::Finish()
{
  for (auto & anim : m_animations)
    anim->Finish();
  Animation::Finish();
}

} // namespace df
