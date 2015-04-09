#include "graphics/uniforms_holder.hpp"

#include "std/utility.hpp"

namespace graphics
{
  bool UniformsHolder::insertValue(ESemantic sem, float value)
  {
    return m_singleFloatHolder.insert(make_pair(sem, value)).second;
  }

  bool UniformsHolder::getValue(ESemantic sem, float & value) const
  {
    single_map_t::const_iterator it = m_singleFloatHolder.find(sem);
    if (it == m_singleFloatHolder.end())
      return false;

    value = it->second;
    return true;
  }
}
