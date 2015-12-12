#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "std/array.hpp"
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
  using TCheckCancelledCallback = function<bool ()>;
  using TIsCountryLoadedByNameFn = function<bool (string const &)>;

  RuleDrawer(TDrawerCallback const & drawerFn, TCheckCancelledCallback const & checkCancelled,
             TIsCountryLoadedByNameFn const & isLoadedFn, ref_ptr<EngineContext> context);
  ~RuleDrawer();

  void operator() (FeatureType const & f);

private:
  bool CheckCancelled();

  TDrawerCallback m_callback;
  TCheckCancelledCallback m_checkCancelled;
  TIsCountryLoadedByNameFn m_isLoadedFn;

  ref_ptr<EngineContext> m_context;
  m2::RectD m_globalRect;
  double m_currentScaleGtoP;

  array<TMapShapes, df::PrioritiesCount> m_mapShapes;
  bool m_wasCancelled;
};

} // namespace dfo
