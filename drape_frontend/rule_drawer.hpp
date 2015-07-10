#pragma once

#include "drape_frontend/tile_key.hpp"

#include "drape/pointers.hpp"

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
class MapShape;

class RuleDrawer
{
public:
  using TDrawerCallback = function<void (FeatureType const &, Stylist &)>;
  RuleDrawer(TDrawerCallback const & fn,
             ref_ptr<EngineContext> context);

  void InsertShape(drape_ptr<MapShape> && shape);

  void operator() (FeatureType const & f);

private:
  TDrawerCallback m_callback;
  ref_ptr<EngineContext> m_context;
  m2::RectD m_globalRect;
  ScreenBase m_geometryConvertor;
  double m_currentScaleGtoP;
  set<string> m_coastlines;

  list<drape_ptr<MapShape>> m_mapShapes;
};

} // namespace dfo
