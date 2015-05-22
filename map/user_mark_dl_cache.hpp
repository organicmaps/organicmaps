#pragma once

#include "base/math.hpp"
#include "graphics/defines.hpp"

#include "std/map.hpp"

namespace graphics
{
  class Screen;
  class DisplayList;
}

class UserMarkDLCache
{
public:
  struct Key
  {
    Key(string const & name, graphics::EPosition anchor, double depthLayer)
      : m_name(name), m_anchor(anchor), m_depthLayer(depthLayer) {}

    string m_name;
    graphics::EPosition m_anchor;
    double m_depthLayer;

    bool operator < (Key const & other) const
    {
      if (m_name != other.m_name)
        return m_name < other.m_name;
      if (!my::AlmostEqualULPs(m_depthLayer, other.m_depthLayer))
        return m_depthLayer < other.m_depthLayer;

      return m_anchor < other.m_anchor;
    }
  };

  UserMarkDLCache(graphics::Screen * cacheScreen);
  ~UserMarkDLCache();

  graphics::DisplayList * FindUserMark(Key const & key);

private:
  graphics::DisplayList * CreateDL(Key const & key);

private:
  graphics::Screen * m_cacheScreen;
  typedef map<Key, graphics::DisplayList *> cache_t;
  typedef cache_t::iterator node_t;
  cache_t m_dls;
};
