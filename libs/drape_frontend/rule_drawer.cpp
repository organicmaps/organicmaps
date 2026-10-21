#include "drape_frontend/rule_drawer.hpp"

#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/path_text_shape.hpp"
#include "drape_frontend/terrain_shade_shape.hpp"
#include "drape_frontend/traffic_renderer.hpp"

#include "drape/drape_diagnostics.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"
#include "indexer/terrain/isolines_tracer.hpp"
#include "indexer/terrain/terrain_utils.hpp"

#include "platform/localization.hpp"
#include "platform/measurement_utils.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/string_utils.hpp"

#if defined(DRAW_TILE_NET) || defined(TERRAIN_DEBUG_MESH)
#include "drape_frontend/text_shape.hpp"

#include <set>
#endif

#include <array>
#include <functional>
#include <vector>

namespace df
{

namespace
{
// The first zoom level in kAverageSegmentsCount.
int constexpr kFirstZoomInAverageSegments = 10;
// clang-format off
std::array<size_t, 10> const kAverageSegmentsCount = {
    // 10  11    12     13    14    15    16    17    18   19
    10000, 5000, 10000, 5000, 2500, 5000, 2000, 1000, 500, 500};
// clang-format on

double constexpr kMetersPerLevel = 3.0;

double GetBuildingHeightInMeters(FeatureType & f)
{
  double constexpr kDefaultHeightInMeters = 3.0;

  double heightInMeters = kDefaultHeightInMeters;

  auto value = f.GetMetadata(feature::Metadata::FMD_HEIGHT);
  if (!value.empty())
  {
    if (!strings::to_double(value, heightInMeters))
      heightInMeters = kDefaultHeightInMeters;
  }
  else
  {
    value = f.GetMetadata(feature::Metadata::FMD_BUILDING_LEVELS);
    if (!value.empty())
    {
      if (strings::to_double(value, heightInMeters))
        heightInMeters *= kMetersPerLevel;
    }
  }
  return heightInMeters;
}

double GetBuildingMinHeightInMeters(FeatureType & f)
{
  double minHeightInMeters = 0.0;

  auto value = f.GetMetadata(feature::Metadata::FMD_MIN_HEIGHT);
  if (!value.empty())
  {
    if (!strings::to_double(value, minHeightInMeters))
      minHeightInMeters = 0.0;
  }
  else
  {
    value = f.GetMetadata(feature::Metadata::FMD_BUILDING_MIN_LEVEL);
    if (!value.empty())
    {
      if (!strings::to_double(value, minHeightInMeters))
        minHeightInMeters = 0.0;
      else
        minHeightInMeters *= kMetersPerLevel;
    }
  }

  return minHeightInMeters;
}

void ExtractTrafficGeometry(FeatureType const & f, df::RoadClass const & roadClass, m2::PolylineD const & polyline,
                            bool oneWay, int zoomLevel, double pixelToGlobalScale,
                            df::TrafficSegmentsGeometry & geometry)
{
  if (polyline.GetSize() < 2)
    return;

  auto const & regionData = f.GetID().m_mwmId.GetInfo()->GetRegionData();
  bool const isLeftHandTraffic = regionData.Get(feature::RegionData::RD_DRIVING) == "l";

  // Calculate offset between road lines in mercator for two-way roads.
  double twoWayOffset = 0.0;
  if (!oneWay)
    twoWayOffset = pixelToGlobalScale * df::TrafficRenderer::GetTwoWayOffset(roadClass, zoomLevel);

  static std::array<uint8_t, 2> const directions = {traffic::TrafficInfo::RoadSegmentId::kForwardDirection,
                                                    traffic::TrafficInfo::RoadSegmentId::kReverseDirection};
  auto & segments = geometry[f.GetID().m_mwmId];

  int const index = zoomLevel - kFirstZoomInAverageSegments;
  ASSERT_GREATER_OR_EQUAL(index, 0, ());
  ASSERT_LESS(index, static_cast<int>(kAverageSegmentsCount.size()), ());
  segments.reserve(kAverageSegmentsCount[index]);

  for (uint16_t segIndex = 0; segIndex + 1 < static_cast<uint16_t>(polyline.GetSize()); ++segIndex)
  {
    for (size_t dirIndex = 0; dirIndex < directions.size(); ++dirIndex)
    {
      if (oneWay && dirIndex > 0)
        break;

      auto const sid = traffic::TrafficInfo::RoadSegmentId(f.GetID().m_index, segIndex, directions[dirIndex]);
      bool isReversed = (directions[dirIndex] == traffic::TrafficInfo::RoadSegmentId::kReverseDirection);

      auto const segment = polyline.ExtractSegment(segIndex, isReversed);
      ASSERT_EQUAL(segment.size(), 2, ());

      // Skip zero-length segments.
      double const kEps = 1e-5;
      if (segment[0].EqualDxDy(segment[1], kEps))
        break;

      if (!oneWay)
      {
        m2::PointD const tangent = (segment[1] - segment[0]).Normalize();
        m2::PointD const normal =
            isLeftHandTraffic ? m2::PointD(-tangent.y, tangent.x) : m2::PointD(tangent.y, -tangent.x);
        m2::PolylineD segmentPolyline(
            std::vector<m2::PointD>{segment[0] + normal * twoWayOffset, segment[1] + normal * twoWayOffset});
        segments.emplace_back(sid, df::TrafficSegmentGeometry(std::move(segmentPolyline), roadClass));
      }
      else
      {
        m2::PolylineD segmentPolyline(std::vector<m2::PointD>{segment[0], segment[1]});
        segments.emplace_back(sid, df::TrafficSegmentGeometry(std::move(segmentPolyline), roadClass));
      }
    }
  }
}

}  // namespace

RuleDrawer::RuleDrawer(TIsCountryLoadedByNameFn const & isLoadedFn, ref_ptr<EngineContext> engineContext,
                       int8_t deviceLang, bool drawTerrain)
  : m_isLoadedFn(isLoadedFn)
  , m_context(engineContext)
  , m_customFeaturesContext(engineContext->GetCustomFeaturesContext().lock())
  , m_mapStyle(GetStyleReader().GetCurrentStyle())
  , m_deviceLang(deviceLang)
  , m_drawTerrain(drawTerrain)
{
  m_applyParams.Init(m_context->GetTileKey());
  m_zoomLevel = m_applyParams.m_tileKey.m_zoomLevel;

  m_mapShapes[df::OverlayType].reserve(200 /* average overlays count */);

  if (m_mapStyle != MapStyleVehicleLight && m_mapStyle != MapStyleVehicleDark)
  {
    /// @todo Make naive implementation for now. Fetch draw settings from EngineContext.
    /// Should refactor and generalize these settings (3D, isolines, hiking, cycling, ...)
    m_relsSettings.Load(m_mapStyle);
  }

  m_applyParams.m_insertShape = [this](drape_ptr<MapShape> && shape)
  {
    size_t const index = shape->GetType();
    ASSERT_LESS(index, m_mapShapes.size(), ());

    /// @todo MinZoom was used for optimization in RenderGroup::UpdateCanBeDeletedStatus(), but is long time
    /// broken. See https://github.com/organicmaps/organicmaps/pull/5903 for details.
    shape->SetFeatureMinZoom(0);
    m_mapShapes[index].push_back(std::move(shape));
  };
}

RuleDrawer::~RuleDrawer()
{
  if (CheckCancelled())
    return;

  auto & overlayShapes = m_mapShapes[df::OverlayType];
  for (auto const & shape : overlayShapes)
    shape->Prepare(m_context->GetTextureManager());

  if (!overlayShapes.empty())
  {
    m_context->FlushOverlays(std::move(overlayShapes));
    overlayShapes.clear();
  }

  m_context->FlushTrafficGeometry(std::move(m_trafficGeometry));
}

bool RuleDrawer::CheckCancelled() const
{
  return m_context->IsCancelled();
}

bool RuleDrawer::IsDiscardCustomFeature(FeatureID const & id) const
{
  return m_customFeaturesContext && m_customFeaturesContext->NeedDiscardGeometry(id);
}

bool RuleDrawer::CheckCoastlines(FeatureType & f)
{
  if (m_zoomLevel > scales::GetUpperWorldScale() && f.GetID().m_mwmId.GetInfo()->GetType() == MwmInfo::COASTS)
  {
    std::string_view const name = f.GetDefaultName();
    if (!name.empty())
    {
      strings::SimpleTokenizer iter(name, ";");
      while (iter)
      {
        if (m_isLoadedFn(*iter))
          return false;
        ++iter;
      }
    }
  }
  return true;
}

void RuleDrawer::ProcessAreaAndPointStyle(FeatureType & f, Stylist const & s)
{
  bool isBuilding = false;
  bool is3dBuilding = false;
  bool isBuildingOutline = false;

  feature::TypesHolder const types(f);
  if (f.GetLayer() >= 0)  // 90% true
  {
    bool const hasParts = m_isBuildingHasParts(types);  // possible to do this checks beforehand in stylist?
    bool const isPart = m_isBuildingPart(types);

    // Looks like nonsense, but there are some osm objects with types
    // highway-path-bridge and building (sic!) at the same time (pedestrian crossing).
    isBuilding = (isPart || m_isBuilding(types)) && !m_isBridgeOrTunnel(types);

    isBuildingOutline = isBuilding && hasParts && !isPart;
    is3dBuilding = isBuilding && !isBuildingOutline && m_context->Is3dBuildingsEnabled();
  }

  m2::PointD featureCenter;

  float areaHeight = 0.0f;
  float areaMinHeight = 0.0f;
  if (is3dBuilding)
  {
    double const heightInMeters = GetBuildingHeightInMeters(f);
    double const minHeightInMeters = GetBuildingMinHeightInMeters(f);
    // Loads geometry of the feature.
    featureCenter = feature::GetCenter(f, m_zoomLevel);
    double const lon = mercator::XToLon(featureCenter.x);
    double const lat = mercator::YToLat(featureCenter.y);

    m2::RectD rectMercator = mercator::MetersToXY(lon, lat, heightInMeters);
    areaHeight = static_cast<float>((rectMercator.SizeX() + rectMercator.SizeY()) * 0.5);

    rectMercator = mercator::MetersToXY(lon, lat, minHeightInMeters);
    areaMinHeight = static_cast<float>((rectMercator.SizeX() + rectMercator.SizeY()) * 0.5);
  }

  bool applyPointStyle = s.m_symbolRule || s.m_captionRule || s.m_houseNumberRule;
  if (applyPointStyle)
  {
    if (!is3dBuilding)
    {
      // Loads geometry of the feature.
      featureCenter = feature::GetCenter(f, m_zoomLevel);
    }
    applyPointStyle = m_applyParams.m_tileRect.IsPointInside(featureCenter);
  }

  bool const skipTriangles = isBuildingOutline && m_context->Is3dBuildingsEnabled();
  if (!skipTriangles && isBuilding && f.GetTrgVerticesCount(m_zoomLevel) >= 10000)
    isBuilding = false;

  // In Satellite mode area fills are drawn at the user-configured opacity so the imagery shows through;
  // 0 hides them entirely (skip the geometry below). Outside Satellite mode areas are always opaque.
  float const areaOpacity =
      m_context->GetBackgroundMode() == dp::BackgroundMode::Satellite ? m_context->GetAreaOpacity() : 1.0f;

  ApplyAreaFeature apply(m_applyParams, f, s.m_captionDescriptor, isBuilding, m_isMwmBorder(types),
                         areaMinHeight /* minPosZ */, areaHeight /* posZ */, areaOpacity);

  if (areaOpacity > 0.0f && !skipTriangles && (s.m_areaRule || s.m_hatchingRule))
  {
    f.ForEachTriangle(apply, m_zoomLevel);
    if (apply.HasGeometry())
    {
      std::string_view hatchKey;
      if (s.m_hatchingRule)
        hatchKey = m_isHatching.GetHatch(types);
      std::string_view patternKey;
      if (s.m_areaRule)
        patternKey = m_isAreaPattern.GetPattern(types);
      apply.ProcessAreaRules(s.m_areaRule, s.m_hatchingRule, hatchKey, patternKey);
    }
  }

  /// @todo Can we put this check in the beginning of this function?
  if (applyPointStyle && !IsDiscardCustomFeature(f.GetID()))
  {
    apply.ProcessPointRules(s.m_symbolRule, s.m_captionRule, s.m_houseNumberRule, featureCenter,
                            m_context->GetTextureManager());
  }
}

void RuleDrawer::ProcessLineStyle(FeatureType & f, Stylist const & s)
{
  bool const isIsoline = m_isIsoline(f);
  ApplyLineFeatureGeometry applyGeom(m_applyParams, f, m_relsSettings);
  applyGeom.BuildGeometry(m_zoomLevel, isIsoline);

  if (applyGeom.HasGeometry())
    applyGeom.ProcessLineRules(s.m_lineRules, isIsoline);

  if (s.m_pathtextRule || s.m_shieldRule)
  {
    std::vector<m2::SharedSpline> clippedSplines;

    auto const metalineSpline = m_context->GetMetalineManager()->GetMetaline(f.GetID());
    if (metalineSpline.IsNull())
    {
      // There is no metaline for this feature.
      clippedSplines = applyGeom.MoveClippedSplines();
    }
    else if (m_usedMetalines.insert(metalineSpline.Get()).second)
    {
      // Generate additional by metaline, mark metaline spline as used.
      clippedSplines = m2::ClipSplineByRect(m_applyParams.m_tileRect, metalineSpline);
    }

    if (!clippedSplines.empty())
    {
      ApplyLineFeatureAdditional applyAdditional(m_applyParams, f, s.m_captionDescriptor, std::move(clippedSplines));
      applyAdditional.ProcessAdditionalLineRules(s.m_pathtextRule, s.m_shieldRule, m_context->GetTextureManager(),
                                                 s.m_roadShields, m_generatedRoadShields);
    }
  }

  if (m_context->IsTrafficEnabled() && m_zoomLevel >= kRoadClass0ZoomLevel)
  {
    using ftypes::HighwayClass;
    struct Checker
    {
      std::vector<HighwayClass> m_highwayClasses;
      int m_zoomLevel;
      df::RoadClass m_roadClass;
    };
    static Checker const checkers[] = {
        {{HighwayClass::Trunk, HighwayClass::Primary}, kRoadClass0ZoomLevel, df::RoadClass::Class0},
        {{HighwayClass::Secondary, HighwayClass::Tertiary}, kRoadClass1ZoomLevel, df::RoadClass::Class1},
        {{HighwayClass::LivingStreet, HighwayClass::Service, HighwayClass::ServiceMinor},
         kRoadClass2ZoomLevel,
         df::RoadClass::Class2}};

    bool const oneWay = m_isOneWay(f);
    auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(f));
    for (size_t i = 0; i < ARRAY_SIZE(checkers); ++i)
    {
      auto const & classes = checkers[i].m_highwayClasses;
      if (find(classes.begin(), classes.end(), highwayClass) != classes.end() && m_zoomLevel >= checkers[i].m_zoomLevel)
      {
        // Need reset, because (possible) simplified geometry was cached before.
        f.ResetGeometry();
        std::vector<m2::PointD> points;
        assign_range(points, f.GetPoints(FeatureType::BEST_GEOMETRY));

        ExtractTrafficGeometry(f, checkers[i].m_roadClass, m2::PolylineD(std::move(points)), oneWay, m_zoomLevel,
                               m_applyParams.m_trafficScalePtoG, m_trafficGeometry);
        break;
      }
    }
  }
}

void RuleDrawer::ProcessPointStyle(FeatureType & f, Stylist const & s)
{
  if (IsDiscardCustomFeature(f.GetID()))
    return;

  ApplyPointFeature apply(m_applyParams, f, s.m_captionDescriptor);
  apply.ProcessPointRules(s.m_symbolRule, s.m_captionRule, s.m_houseNumberRule, f.GetCenter(),
                          m_context->GetTextureManager());
}

void RuleDrawer::operator()(FeatureType & f)
{
  if (CheckCancelled())
    return;

  feature::TypesHolder const types(f);
  // m_drawTerrain suppresses the baked isoline features for the WHOLE tile while the
  // dynamic ones cover only the terrain blocks: a tile straddling the coverage border
  // shows a one-tile-wide gap. Accepted: per-feature gating against the mesh rects
  // costs more than the artifact is worth, and the coverage grows block by block.
  if (((!m_context->IsolinesEnabled() || m_drawTerrain) && m_isIsoline(types)) ||
      (!m_context->Is3dBuildingsEnabled() && m_isBuildingPart(types) && !m_isBuilding(types)))
    return;

  if (m_isCoastline(types) && !CheckCoastlines(f))
    return;

  feature::GeomType const geomType = f.GetGeomType();

  // Outdoors rules give hiking/cycling relation members consistent low-zoom visibility, so routes
  // remain continuous even when their member roads have different visibility in the base style.
  auto style = m_mapStyle;
  if (m_applyParams.IsRelationRoutes() && geomType == feature::GeomType::Line && !m_relsSettings.IsEmpty())
  {
    RelationsDrawInfo drawInfo(m_relsSettings);
    if (drawInfo.HasHikingOrCycling(f))
      style = MapStyleIsDark(m_mapStyle) ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  }

  Stylist const s(f, m_zoomLevel, m_deviceLang, style);

  // No drawing rules.
  if (!s.m_symbolRule && !s.m_captionRule && !s.m_houseNumberRule && s.m_lineRules.empty() && !s.m_areaRule &&
      !s.m_hatchingRule)
    return;

#ifdef DEBUG
  // Validate mixing of feature styles.
  bool const hasLine = !s.m_lineRules.empty();
  bool const hasLineAdd = s.m_shieldRule || s.m_pathtextRule;
  bool const hasPoint = s.m_symbolRule || s.m_captionRule || s.m_houseNumberRule;
  bool const hasArea = s.m_areaRule || s.m_hatchingRule;

  ASSERT(!((hasLine || hasLineAdd) && (hasPoint || hasArea)),
         ("Line drules mixed with point/area ones", f.DebugString()));
  ASSERT(!hasLineAdd || hasLine, ("Pathtext/shield without a line drule", f.DebugString()));
#endif

  // FeatureType::GetLimitRect call invokes full geometry reading and decoding.
  // That's why this code follows after all lightweight return options.
  if (!m_applyParams.m_tileRect.IsIntersect(f.GetLimitRect(m_zoomLevel)))
    return;

  if (geomType == feature::GeomType::Area)
  {
    ProcessAreaAndPointStyle(f, s);
  }
  else if (!s.m_lineRules.empty())
  {
    ASSERT(geomType == feature::GeomType::Line, ());
    ProcessLineStyle(f, s);
  }
  else
  {
    ASSERT(s.m_symbolRule || s.m_captionRule || s.m_houseNumberRule, ());
    ASSERT(geomType == feature::GeomType::Point, ());
    ProcessPointStyle(f, s);
  }

  if (CheckCancelled())
    return;

  FlushGeometryShapes();
}

void RuleDrawer::FlushGeometryShapes()
{
  auto & shapes = m_mapShapes[df::GeometryType];
  for (auto const & shape : shapes)
    shape->Prepare(m_context->GetTextureManager());

  if (!shapes.empty())
    m_context->Flush(std::move(shapes));
}

void RuleDrawer::DrawTerrain(MapDataProvider const & model)
{
  ASSERT(m_drawTerrain, ());
  if (CheckCancelled())
    return;

  // The isolines drawing policy (trace step, line and label rules per altitude) is
  // resolved from the current style once per tile; the units are read once here, so
  // the traced levels, the labels and the steps always agree.
  auto const units = measurement_utils::GetMeasurementUnits();
  terrain::IsolinesStyle const isolinesStyle(m_zoomLevel, units);
  /// @todo Separate visibility settings for the isolines and the hillshading.
  bool const drawIsolines = isolinesStyle.GetStep() != 0;

  // One read serves the hillshading, the isolines and the debug mesh. The rect is
  // inflated for the isolines smoothing overhang (see ClipSplinesBuilder::Release);
  // it also completes the shade normals of the vertices near the tile border.
  m2::RectD const queryRect = GetTerrainReadRect(m_applyParams.m_tileRect);

  terrain::TerrainData data;
  model.ReadTerrain(queryRect, m_zoomLevel, data, m_context->GetCancellable());
  if (CheckCancelled() || data.IsEmpty())
    return;

#ifdef TERRAIN_DEBUG_MESH
  DrawTerrainDebugMesh(data.m_mesh);
#else
  DrawTerrainShade(data);
#endif
  if (drawIsolines && !CheckCancelled())
    DrawDynamicIsolines(data, isolinesStyle, units);

  if (CheckCancelled())
    return;

  FlushGeometryShapes();
}

#ifdef TERRAIN_DEBUG_MESH
void RuleDrawer::DrawTerrainDebugMesh(terrain::TileMesh const & mesh)
{
  // The half-open tile ownership draws every edge and label exactly once across the
  // neighbor tiles; the mesh vertices and edges are already deduplicated.
  m2::RectD const & tileRect = m_applyParams.m_tileRect;
  auto const ownedByTile = [&tileRect](m2::PointD const & p)
  { return p.x >= tileRect.minX() && p.x < tileRect.maxX() && p.y >= tileRect.minY() && p.y < tileRect.maxY(); };

  auto const & points = mesh.GetPoints();
  auto const & triangles = mesh.GetTriangles();

  std::set<std::pair<uint32_t, uint32_t>> edges;
  for (size_t t = 0; t + 2 < triangles.size(); t += 3)
    for (int i = 0; i < 3; ++i)
      edges.emplace(std::min(triangles[t + i], triangles[t + (i + 1) % 3]),
                    std::max(triangles[t + i], triangles[t + (i + 1) % 3]));

  for (auto const & [a, b] : edges)
  {
    m2::PointD const & pa = points[a];
    m2::PointD const & pb = points[b];
    if (!ownedByTile((pa + pb) * 0.5))
      continue;
    df::LineViewParams params;
    params.m_tileCenter = tileRect.Center();
    params.m_baseGtoPScale = m_applyParams.m_currentScaleGtoP;
    params.m_cap = dp::ButtCap;
    params.m_join = dp::RoundJoin;
    params.m_color = dp::Color(255, 0, 0, 160);
    params.m_depth = dp::kMaxDepth;
    params.m_depthLayer = DepthLayer::GeometryLayer;
    params.m_width = 1;
    params.m_zoomLevel = m_zoomLevel;
    m_applyParams.m_insertShape(make_unique_dp<LineShape>(m2::SharedSpline(std::vector<m2::PointD>{pa, pb}), params));
  }

  // The running text index keeps the per-tile OverlayIDs unique (cf. the isoline
  // labels starting at 128).
  double const visScale = m_applyParams.m_vparams.GetVisualScale();
  uint32_t textIndex = 1 << 16;
  for (size_t i = 0; i < points.size(); ++i)
  {
    if (!ownedByTile(points[i]))
      continue;
    df::TextViewParams tp;
    tp.m_markId = kml::kDebugMarkId;
    tp.m_tileCenter = tileRect.Center();
    tp.m_titleDecl.m_anchor = dp::Center;
    tp.m_depth = dp::kMaxDepth;
    tp.m_depthLayer = DepthLayer::OverlayLayer;
    tp.m_titleDecl.m_primaryText = strings::to_string(mesh.GetAltitudes()[i]);
    tp.m_titleDecl.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 10 * visScale);
    tp.m_titleDecl.m_primaryOffset = {0.0f, 0.0f};
    auto textShape = make_unique_dp<TextShape>(points[i], tp, m_context->GetTileKey(), m2::PointF(0.0f, 0.0f),
                                               m2::PointF(0.0f, 0.0f), dp::Anchor::Center, textIndex++);
    textShape->DisableDisplacing();
    m_applyParams.m_insertShape(std::move(textShape));
  }
}
#endif

void RuleDrawer::DrawTerrainShade(terrain::TerrainData const & data)
{
  auto geometry = BuildTerrainShade(data, m_applyParams.m_tileRect, m_context->GetCancellable());
  if (!geometry.m_triangles.empty())
    m_applyParams.m_insertShape(make_unique_dp<TerrainShadeShape>(std::move(geometry.m_triangles)));
}

void RuleDrawer::DrawDynamicIsolines(terrain::TerrainData const & data, terrain::IsolinesStyle const & isolinesStyle,
                                     measurement_utils::Units units)
{
  double const visScale = m_applyParams.m_vparams.GetVisualScale();
  ClipSplinesBuilder builder(m_applyParams);

  // All the dynamic labels of a tile share an invalid FeatureID, so their OverlayID
  // uniqueness rests entirely on the running text index (cf. kPathTextBaseTextIndex
  // of the baked path texts); each shape reserves one index per repeated placement.
  uint32_t textIndex = 128;

  auto const unitSuffix = units == measurement_utils::Units::Imperial ? " " + platform::GetLocalizedString("ft") : "";
  // The isolines come in the display units (see terrain::TraceIsolines).
  terrain::TraceIsolines(data, isolinesStyle.GetStep(), units, [&](terrain::Isoline && isoline)
  {
    if (CheckCancelled())
      return;
    auto const * lineRule = isolinesStyle.GetLineRule(isoline.m_altitude);
    if (lineRule == nullptr)
      return;
    auto const * textRule = isolinesStyle.GetPathTextRule(isoline.m_altitude);

    builder.SetPath(std::move(isoline.m_points));
    for (auto const & spline : builder.Release(true /* isIsoline */))
    {
      LineViewParams params;
      params.m_tileCenter = m_applyParams.m_tileRect.Center();
      ExtractLineParams(*lineRule, visScale, params);

      /// @todo Add depth contours (negative altitudes) to the styles.
      if (isoline.m_altitude < 0)
        params.m_color = dp::Color(38, 118, 196, params.m_color.GetAlpha());

      // Just below the hillshade (see df::BuildTerrainShade), which is drawn after the lines: the shade
      // blends over the translucent contours instead of being depth-tested out underneath them.
      params.m_depth = drule::kMinLayeredDepthFg - 1;
      params.m_depthLayer = DepthLayer::GeometryLayer;
      params.m_baseGtoPScale = m_applyParams.m_currentScaleGtoP;
      params.m_zoomLevel = m_zoomLevel;
      m_applyParams.m_insertShape(make_unique_dp<LineShape>(spline, params));

      if (textRule == nullptr)
        continue;

      auto const & caption = *textRule->primary;
      PathTextViewParams textParams;
      textParams.m_tileCenter = m_applyParams.m_tileRect.Center();
      textParams.m_depthLayer = DepthLayer::OverlayLayer;
      textParams.m_depthTestEnabled = false;
      // Pathtext drule priorities map to the overlay depth directly.
      textParams.m_depth = textRule->priority;
      textParams.m_mainText = strings::to_string(isoline.m_altitude) + unitSuffix;
      CaptionDefToFontDecl(&caption, visScale, textParams.m_textFont);

      /// @todo Add depth contours (negative altitudes) to the styles.
      if (isoline.m_altitude < 0)
        textParams.m_textFont.m_color = dp::Color(25, 96, 168, 255);

      textParams.m_baseGtoPScale = m_applyParams.m_currentScaleGtoP;

      auto shape = make_unique_dp<PathTextShape>(spline, textParams, m_applyParams.m_tileKey, textIndex);
      // The layout fails on splines too short for the text: no label then.
      if (!shape->CalculateLayout(m_context->GetTextureManager()))
        continue;
      textIndex += static_cast<uint32_t>(shape->GetOffsets().size());
      m_applyParams.m_insertShape(std::move(shape));
    }
  }, m_context->GetCancellable());
}

#ifdef DRAW_TILE_NET
void RuleDrawer::DrawTileNet()
{
  if (CheckCancelled())
    return;

  auto const key = m_context->GetTileKey();

  std::vector<m2::PointD> path;
  path.reserve(4);
  path.push_back(m_applyParams.m_tileRect.LeftBottom());
  path.push_back(m_applyParams.m_tileRect.LeftTop());
  path.push_back(m_applyParams.m_tileRect.RightTop());
  path.push_back(m_applyParams.m_tileRect.RightBottom());

  m2::SharedSpline spline(path);
  df::LineViewParams p;
  p.m_tileCenter = m_applyParams.m_tileRect.Center();
  p.m_baseGtoPScale = 1.0;
  p.m_cap = dp::ButtCap;
  p.m_color = dp::Color::Blue();
  p.m_depth = dp::kMaxDepth;
  p.m_depthLayer = DepthLayer::GeometryLayer;
  p.m_width = 1;
  p.m_join = dp::RoundJoin;

  auto lineShape = make_unique_dp<LineShape>(spline, p);
  lineShape->Prepare(m_context->GetTextureManager());
  TMapShapes shapes;
  shapes.push_back(std::move(lineShape));
  m_context->Flush(std::move(shapes));

  df::TextViewParams tp;
  tp.m_markId = kml::kDebugMarkId;
  tp.m_tileCenter = p.m_tileCenter;
  tp.m_titleDecl.m_anchor = dp::Center;
  tp.m_depth = dp::kMaxDepth;
  tp.m_depthLayer = DepthLayer::OverlayLayer;
  tp.m_titleDecl.m_primaryText = key.Coord2String();

  tp.m_titleDecl.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 30);
  tp.m_titleDecl.m_primaryOffset = {0.0f, 0.0f};
  auto textShape =
      make_unique_dp<TextShape>(p.m_tileCenter, tp, key, m2::PointF(0.0f, 0.0f) /* symbolSize */,
                                m2::PointF(0.0f, 0.0f) /* symbolOffset */, dp::Anchor::Center, 0 /* textIndex */);
  textShape->DisableDisplacing();

  textShape->Prepare(m_context->GetTextureManager());
  TMapShapes overlayShapes;
  overlayShapes.push_back(std::move(textShape));
  m_context->FlushOverlays(std::move(overlayShapes));
}
#endif
}  // namespace df
