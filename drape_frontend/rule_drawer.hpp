#pragma once

#include "drape_frontend/custom_features_context.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "indexer/road_shields_parser.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <array>
#include <functional>
#include <map>
#include <string>
#include <unordered_set>

class FeatureType;

namespace df
{
class EngineContext;
class Stylist;

class RuleDrawer
{
public:
  using TDrawerCallback = std::function<void(FeatureType &, Stylist &)>;
  using TCheckCancelledCallback = std::function<bool()>;
  using TIsCountryLoadedByNameFn = std::function<bool(std::string const &)>;
  using TInsertShapeFn = std::function<void(drape_ptr<MapShape> && shape)>;
  using TFilterFeatureFn = std::function<bool(FeatureType &)>;

  RuleDrawer(TDrawerCallback const & drawerFn,
             TCheckCancelledCallback const & checkCancelled,
             TIsCountryLoadedByNameFn const & isLoadedFn,
             TFilterFeatureFn const & filterFn,
             ref_ptr<EngineContext> engineContext);
  ~RuleDrawer();

  void operator()(FeatureType & f);

#ifdef DRAW_TILE_NET
  void DrawTileNet();
#endif

private:
  void ProcessAreaStyle(FeatureType & f, Stylist const & s, TInsertShapeFn const & insertShape,
                        int & minVisibleScale);
  void ProcessLineStyle(FeatureType & f, Stylist const & s, TInsertShapeFn const & insertShape,
                        int & minVisibleScale);
  void ProcessPointStyle(FeatureType & f, Stylist const & s, TInsertShapeFn const & insertShape,
                         int & minVisibleScale);

  bool CheckCoastlines(FeatureType & f, Stylist const & s);

  bool CheckCancelled();

  TDrawerCallback m_callback;
  TCheckCancelledCallback m_checkCancelled;
  TIsCountryLoadedByNameFn m_isLoadedFn;
  TFilterFeatureFn m_filter;

  ref_ptr<EngineContext> m_context;
  CustomFeaturesContextPtr m_customFeaturesContext;
  std::unordered_set<m2::Spline const *> m_usedMetalines;

  m2::RectD m_globalRect;
  double m_currentScaleGtoP;
  double m_trafficScalePtoG;

  TrafficSegmentsGeometry m_trafficGeometry;

  std::array<TMapShapes, df::MapShapeTypeCount> m_mapShapes;
  bool m_wasCancelled;

  GeneratedRoadShields m_generatedRoadShields;
};
}  // namespace df
