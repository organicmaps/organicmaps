#include "animation.hpp"

namespace df
{

bool Animation::CouldBeBlendedWith(Animation const & animation) const
{
  return (GetType() != animation.GetType()) &&
      m_couldBeBlended && animation.m_couldBeBlended;
}

} // namespace df
