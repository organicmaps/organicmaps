#include "parallel_animation.hpp"

#include "animation_system.hpp"

namespace df
{

ParallelAnimation::ParallelAnimation()
  : Animation(true /* couldBeInterrupted */, true /* couldBeBlended */)
{}

string ParallelAnimation::GetCustomType() const
{
  return m_customType;
}

void ParallelAnimation::SetCustomType(string const & type)
{
  m_customType = type;
}

Animation::TAnimObjects const & ParallelAnimation::GetObjects() const
{
  return m_objects;
}

bool ParallelAnimation::HasObject(TObject object) const
{
  return m_objects.find(object) != m_objects.end();
}

Animation::TObjectProperties const & ParallelAnimation::GetProperties(TObject object) const
{
  ASSERT(HasObject(object), ());
  return m_properties.find(object)->second;
}

bool ParallelAnimation::HasProperty(TObject object, TProperty property) const
{
  if (!HasObject(object))
    return false;
  TObjectProperties const & properties = GetProperties(object);
  return properties.find(property) != properties.end();
}

bool ParallelAnimation::HasTargetProperty(TObject object, TProperty property) const
{
  ASSERT(!m_animations.empty(), ());
  for (auto const & anim : m_animations)
  {
    if (anim->HasTargetProperty(object, property))
      return true;
  }
  return false;
}

void ParallelAnimation::SetMaxDuration(double maxDuration)
{
  for (auto const & anim : m_animations)
    anim->SetMaxDuration(maxDuration);
}

double ParallelAnimation::GetDuration() const
{
  double duration = 0.0;
  for (auto const & anim : m_animations)
    duration = max(duration, anim->GetDuration());
  return duration;
}

bool ParallelAnimation::IsFinished() const
{
  return m_animations.empty();
}

bool ParallelAnimation::GetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT(!m_animations.empty(), ());
  for (auto const & anim : m_animations)
  {
    if (anim->HasProperty(object, property))
      return anim->GetProperty(object, property, value);
  }
  return false;
}

bool ParallelAnimation::GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const
{
  ASSERT(!m_animations.empty(), ());
  for (auto const & anim : m_animations)
  {
    if (anim->HasProperty(object, property))
      return anim->GetTargetProperty(object, property, value);
  }
  return false;
}

void ParallelAnimation::AddAnimation(drape_ptr<Animation> && animation)
{
  SetCouldBeInterrupted(CouldBeInterrupted() && animation->CouldBeInterrupted());
  SetCouldBeBlended(CouldBeBlended() && animation->CouldBeBlended());
  SetCouldBeRewinded(CouldBeRewinded() && animation->CouldBeRewinded());

  m_animations.emplace_back(move(animation));
  ObtainObjectProperties();
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
      AnimationSystem::Instance().SaveAnimationResult(*(*iter));
      iter = m_animations.erase(iter);
      ObtainObjectProperties();
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
  {
    anim->Finish();
    AnimationSystem::Instance().SaveAnimationResult(*anim);
  }
  m_animations.clear();
  ObtainObjectProperties();
  Animation::Finish();
}

void ParallelAnimation::ObtainObjectProperties()
{
  m_objects.clear();
  m_properties.clear();

  if (m_animations.empty())
    return;

  for (auto const & anim : m_animations)
  {
    TAnimObjects const & objects = anim->GetObjects();
    m_objects.insert(objects.begin(), objects.end());
    for (auto const & object : objects)
    {
      TObjectProperties const & properties = anim->GetProperties(object);
      m_properties[object].insert(properties.begin(), properties.end());
    }
  }
}

} // namespace df
