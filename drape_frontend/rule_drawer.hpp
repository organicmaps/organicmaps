#pragma once

#include "drape_frontend/tile_key.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "std/function.hpp"
#include "std/set.hpp"
#include "std/string.hpp"

class FeatureType;

namespace df
{

class EngineContext;
class Stylist;

class RuleDrawer
{
public:
  using TDrawerCallback = function<void (FeatureType const &, Stylist &)>;
  RuleDrawer(TDrawerCallback const & fn,
             EngineContext & context);

  void operator() (FeatureType const & f);

private:
  TDrawerCallback m_callback;
  EngineContext & m_context;
  m2::RectD m_globalRect;
  ScreenBase m_geometryConvertor;
  double m_currentScaleGtoP;
  set<string> m_coastlines;
};

} // namespace dfo
