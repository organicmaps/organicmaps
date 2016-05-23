#include "animation.hpp"

namespace df
{

bool Animation::CouldBeBlendedWith(Animation const & animation) const
{
  bool hasSameObject = false;
  TAnimObjects const & objects = animation.GetObjects();
  for (auto const & object : objects)
  {
    if (HasObject(object))
    {
      hasSameObject = true;
      break;
    }
  }

  return !hasSameObject || ((GetType() != animation.GetType()) &&
      m_couldBeBlended && animation.m_couldBeBlended);
}

} // namespace df
