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

/*
 * RuleDrawer() is invoked for each feature in the tile.
 * It creates a Stylist which filters suitable drawing rules for the feature.
 * Then passes on the drawing rules to ApplyPoint/Area/LineFeature objects
 * which create corresponding MapShape objects (which might in turn create OverlayHandles).
 * The RuleDrawer flushes geometry MapShapes immediately for each feature,
 * while overlay MapShapes are flushed altogether after all features are processed.
 */
class RuleDrawer
{
public:
  using TCheckCancelledCallback = std::function<bool()>;
  using TIsCountryLoadedByNameFn = std::function<bool(std::string_view)>;
  using TInsertShapeFn = std::function<void(drape_ptr<MapShape> && shape)>;

  RuleDrawer(TCheckCancelledCallback const & checkCancelled, TIsCountryLoadedByNameFn const & isLoadedFn,
             ref_ptr<EngineContext> engineContext, int8_t deviceLang);
  ~RuleDrawer();

  void operator()(FeatureType & f);

#ifdef DRAW_TILE_NET
  void DrawTileNet();
#endif

private:
  void ProcessAreaAndPointStyle(FeatureType & f, Stylist const & s, TInsertShapeFn const & insertShape);
  void ProcessLineStyle(FeatureType & f, Stylist const & s, TInsertShapeFn const & insertShape);
  void ProcessPointStyle(FeatureType & f, Stylist const & s, TInsertShapeFn const & insertShape);

  bool CheckCoastlines(FeatureType & f);

  bool CheckCancelled();

  bool IsDiscardCustomFeature(FeatureID const & id) const;

  TCheckCancelledCallback m_checkCancelled;
  TIsCountryLoadedByNameFn m_isLoadedFn;

  ref_ptr<EngineContext> m_context;
  CustomFeaturesContextPtr m_customFeaturesContext;
  int8_t m_deviceLang;
  std::unordered_set<m2::Spline const *> m_usedMetalines;

  m2::RectD m_globalRect;
  double m_currentScaleGtoP;
  double m_trafficScalePtoG;

  TrafficSegmentsGeometry m_trafficGeometry;

  std::array<TMapShapes, df::MapShapeTypeCount> m_mapShapes;

  GeneratedRoadShields m_generatedRoadShields;

  uint8_t m_zoomLevel = 0;
  bool m_wasCancelled = false;
};
}  // namespace df
