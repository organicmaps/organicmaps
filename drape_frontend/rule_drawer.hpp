#pragma once

#include "tile_key.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../std/function.hpp"
#include "../std/set.hpp"
#include "../std/string.hpp"

class FeatureType;

namespace df
{
  class EngineContext;
  class Stylist;
  typedef function<void (FeatureType const &, Stylist &)> drawer_callback_fn;

  class RuleDrawer
  {
  public:
    RuleDrawer(drawer_callback_fn const & fn,
               TileKey const & tileKey,
               EngineContext & context);

    void operator() (FeatureType const & f);

  private:
    drawer_callback_fn m_callback;
    TileKey m_tileKey;
    EngineContext & m_context;
    m2::RectD m_globalRect;
    ScreenBase m_geometryConvertor;
    set<string> m_coastlines;
  };
}
