#pragma once

#include "drape_frontend/custom_symbol.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <array>
#include <functional>
#include <string>

class FeatureType;

namespace df
{
class EngineContext;
class Stylist;

class RuleDrawer
{
public:
  using TDrawerCallback = std::function<void(FeatureType const &, Stylist &)>;
  using TCheckCancelledCallback = std::function<bool()>;
  using TIsCountryLoadedByNameFn = std::function<bool(std::string const &)>;

  RuleDrawer(TDrawerCallback const & drawerFn,
             TCheckCancelledCallback const & checkCancelled,
             TIsCountryLoadedByNameFn const & isLoadedFn,
             ref_ptr<EngineContext> engineContext);
  ~RuleDrawer();

  void operator()(FeatureType const & f);

private:
  bool CheckCancelled();

  TDrawerCallback m_callback;
  TCheckCancelledCallback m_checkCancelled;
  TIsCountryLoadedByNameFn m_isLoadedFn;

  ref_ptr<EngineContext> m_context;
  CustomSymbolsContextPtr m_customSymbolsContext;
  m2::RectD m_globalRect;
  double m_currentScaleGtoP;
  double m_trafficScalePtoG;

  TrafficSegmentsGeometry m_trafficGeometry;

  std::array<TMapShapes, df::MapShapeTypeCount> m_mapShapes;
  bool m_wasCancelled;
};
}  // namespace df
