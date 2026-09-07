#pragma once

#include "drape_frontend/apply_feature_params.hpp"
#include "drape_frontend/custom_features_context.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/relations_draw_info.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/traffic_generator.hpp"

#include "drape/pointers.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/road_shields_parser.hpp"

#include "geometry/spline.hpp"

#include "platform/measurement_utils.hpp"

#include <array>
#include <functional>
#include <unordered_set>

class FeatureType;

namespace terrain
{
class IsolinesStyle;
class TileMesh;
}  // namespace terrain

namespace df
{
class EngineContext;
class MapDataProvider;

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

  RuleDrawer(TCheckCancelledCallback const & checkCancelled, TIsCountryLoadedByNameFn const & isLoadedFn,
             ref_ptr<EngineContext> engineContext, int8_t deviceLang, bool drawTerrain = false);
  ~RuleDrawer();

  void operator()(FeatureType & f);

  /// The terrain layers of the tile - the hillshading and the dynamic isolines (and the
  /// debug mesh instead of the hillshading under TERRAIN_DEBUG_MESH) - drawn over ONE
  /// shared tile mesh read: the isolines drawing policy is resolved from the current
  /// style once, and the mesh is decoded once for all the consumers.
  void DrawTerrain(MapDataProvider const & model);

#ifdef DRAW_TILE_NET
  void DrawTileNet();
#endif

private:
  void ProcessAreaAndPointStyle(FeatureType & f, Stylist const & s);
  void ProcessLineStyle(FeatureType & f, Stylist const & s);
  void ProcessPointStyle(FeatureType & f, Stylist const & s);

  bool CheckCoastlines(FeatureType & f);

  bool CheckCancelled();

  bool IsDiscardCustomFeature(FeatureID const & id) const;

  /// Smooth terrain hillshading: the area-weighted per-vertex normals over the shared
  /// mesh give the Lambert intensity interpolated by the terrain shade shader.
  void DrawTerrainShade(terrain::TileMesh const & mesh);
  /// Traces the dynamic isolines over the shared mesh and emits line shapes through the
  /// same smoothing/clipping pipeline as the baked isoline features (which are
  /// suppressed when the dynamic isolines are used).
  void DrawDynamicIsolines(terrain::TileMesh const & mesh, terrain::IsolinesStyle const & isolinesStyle,
                           measurement_utils::Units units);
  /// The raw mesh inspection: the triangle edges and the vertex altitudes in red.
  void DrawTerrainDebugMesh(terrain::TileMesh const & mesh);

  TCheckCancelledCallback m_checkCancelled;
  TIsCountryLoadedByNameFn m_isLoadedFn;

  ref_ptr<EngineContext> m_context;
  CustomFeaturesContextPtr m_customFeaturesContext;
  std::unordered_set<m2::Spline const *> m_usedMetalines;

  TrafficSegmentsGeometry m_trafficGeometry;

  std::array<TMapShapes, df::MapShapeTypeCount> m_mapShapes;

  GeneratedRoadShields m_generatedRoadShields;

  RelationsDrawSettings m_relsSettings;

  df::ApplyFeatureParams m_applyParams;

  uint8_t m_zoomLevel;
  int8_t m_deviceLang;
  bool m_drawTerrain = false;
  bool m_wasCancelled = false;

  ftypes::IsBuildingHasPartsChecker const & m_isBuildingHasParts = ftypes::IsBuildingHasPartsChecker::Instance();
  ftypes::IsBuildingPartChecker const & m_isBuildingPart = ftypes::IsBuildingPartChecker::Instance();
  ftypes::IsBuildingChecker const & m_isBuilding = ftypes::IsBuildingChecker::Instance();
  ftypes::IsBridgeOrTunnelChecker m_isBridgeOrTunnel;
  ftypes::IsMwmBorderChecker const & m_isMwmBorder = ftypes::IsMwmBorderChecker::Instance();
  IsHatchingTerritoryChecker const & m_isHatching = IsHatchingTerritoryChecker::Instance();
  IsAreaPatternChecker const & m_isAreaPattern = IsAreaPatternChecker::Instance();
  ftypes::IsOneWayChecker const & m_isOneWay = ftypes::IsOneWayChecker::Instance();
  ftypes::IsIsolineChecker const & m_isIsoline = ftypes::IsIsolineChecker::Instance();
  ftypes::IsCoastlineChecker const & m_isCoastline = ftypes::IsCoastlineChecker::Instance();
};
}  // namespace df
