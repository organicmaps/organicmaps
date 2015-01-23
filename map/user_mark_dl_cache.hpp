#pragma once

#include "drape/drape_global.hpp"

#include "base/math.hpp"

#include "std/map.hpp"

class UserMarkDLCache
{
public:
  struct Key
  {
    Key(string const & name, dp::Anchor anchor, double depthLayer)
      : m_name(name), m_anchor(anchor), m_depthLayer(depthLayer) {}

    string m_name;
    dp::Anchor m_anchor;
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

  UserMarkDLCache();
  ~UserMarkDLCache();

  ///@TODO UVR
  //graphics::DisplayList * FindUserMark(Key const & key);

private:
  //graphics::DisplayList * CreateDL(Key const & key);

private:
  //graphics::Screen * m_cacheScreen;
};
