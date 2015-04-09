#pragma once

#include "graphics/defines.hpp"

#include "std/map.hpp"

namespace graphics
{
  class UniformsHolder
  {
  public:
    bool insertValue(ESemantic sem, float value);
    bool getValue(ESemantic sem, float & value) const;

  private:
    typedef map<ESemantic, float> single_map_t;
    single_map_t m_singleFloatHolder;
  };
}

