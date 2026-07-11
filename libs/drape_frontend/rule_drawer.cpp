#include "drape_frontend/rule_drawer.hpp"

#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/clip_splines_builder.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/traffic_renderer.hpp"

#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"
#include "indexer/terrain/terrain_utils.hpp"

#include "coding/point_coding.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include "base/assert.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/path_text_shape.hpp"

#ifdef DRAW_TILE_NET
#include "drape/drape_diagnostics.hpp"

#include "drape_frontend/text_shape.hpp"

#include "base/string_utils.hpp"
#endif

#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <vector>

namespace df
{

namespace
{
// The first zoom level in kAverageSegmentsCount.
int constexpr kFirstZoomInAverageSegments = 10;
std::array<size_t, 10> const kAverageSegmentsCount = {
    // 10  11    12     13    14    15    16    17    18   19
    10000, 5000, 10000, 5000, 2500, 5000, 2000, 1000, 500, 500};

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

RuleDrawer::RuleDrawer(TCheckCancelledCallback const & checkCancelled, TIsCountryLoadedByNameFn const & isLoadedFn,
                       ref_ptr<EngineContext> engineContext, int8_t deviceLang, bool drawDynamicIsolines)
  : m_checkCancelled(checkCancelled)
  , m_isLoadedFn(isLoadedFn)
  , m_context(engineContext)
  , m_customFeaturesContext(engineContext->GetCustomFeaturesContext().lock())
  , m_deviceLang(deviceLang)
  , m_drawDynamicIsolines(drawDynamicIsolines)
{
  ASSERT(m_checkCancelled != nullptr, ());

  m_applyParams.Init(m_context->GetTileKey());
  m_zoomLevel = m_applyParams.m_tileKey.m_zoomLevel;

  m_mapShapes[df::OverlayType].reserve(200 /* average overlays count */);

  if (!GetStyleReader().IsCarNavigationStyle())
  {
    /// @todo Make naive implementation for now. Fetch draw settings from EngineContext.
    /// Should refactor and generalize these settings (3D, isolines, hiking, cycling, ...)
    m_relsSettings.Load();
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
  if (m_wasCancelled)
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

bool RuleDrawer::CheckCancelled()
{
  m_wasCancelled = m_checkCancelled();
  return m_wasCancelled;
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
  if (((!m_context->IsolinesEnabled() || m_drawDynamicIsolines) && m_isIsoline(types)) ||
      (!m_context->Is3dBuildingsEnabled() && m_isBuildingPart(types) && !m_isBuilding(types)))
    return;

  if (m_isCoastline(types) && !CheckCoastlines(f))
    return;

  feature::GeomType const geomType = f.GetGeomType();

  // Force use Outdoor style (mainly because of visibility), for the hiking/cycling related Features
  // (has correspondent Relation references). Otherwise, we get routes torn to separate pieces
  // (e.g. highway=path is visible from z15, but highway=secondary from z13 in a regular Map style).
  bool forceOutdoorStyle = false;
  if (m_applyParams.IsRelationRoutes() && geomType == feature::GeomType::Line && !m_relsSettings.IsEmpty())
  {
    RelationsDrawInfo drawInfo(m_relsSettings);
    if (drawInfo.HasHikingOrCycling(f))
      forceOutdoorStyle = true;
  }

  Stylist const s(f, m_zoomLevel, m_deviceLang, forceOutdoorStyle);

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

  for (auto const & shape : m_mapShapes[df::GeometryType])
    shape->Prepare(m_context->GetTextureManager());

  if (!m_mapShapes[df::GeometryType].empty())
  {
    TMapShapes geomShapes;
    geomShapes.swap(m_mapShapes[df::GeometryType]);
    m_context->Flush(std::move(geomShapes));
  }
}

void RuleDrawer::DrawTerrainShade(MapDataProvider const & model)
{
  if (CheckCancelled())
    return;

  m2::RectD const & tileRect = m_applyParams.m_tileRect;

  // The shade palette: quantized Lambert intensity mapped to semi-transparent black
  // (shadows) and white (highlights) blended over the map fills. The colors live in the
  // dynamic drape palette atlas, so the palette must stay small and fixed.
  int constexpr kShadowBuckets = 24;
  int constexpr kHighlightBuckets = 8;
  uint8_t constexpr kShadowMaxAlpha = 96;
  uint8_t constexpr kHighlightMaxAlpha = 40;
  // The light direction (towards the light): azimuth 315 (NW), altitude 45 degrees;
  // +x = east, +y = north in mercator. Flat ground gets sin(45) intensity, the shade
  // buckets are relative to it, so the flat ground is not drawn at all.
  double constexpr kLightX = -0.5, kLightY = 0.5, kLightZ = 0.70710678;
  double constexpr kFlatIntensity = 0.70710678;
  double constexpr kExaggeration = 1.0;

  // Ground meters per mercator unit at the tile latitude (the projection is conformal:
  // the x and y local scales are equal); the altitudes are meters.
  m2::PointD const center = tileRect.Center();
  double const zScale =
      kExaggeration / mercator::DistanceOnEarth({center.x - 0.5, center.y}, {center.x + 0.5, center.y});

  // The bucket index is shifted by kShadowBuckets: [0, kShadowBuckets) are the shadows.
  std::array<std::vector<m2::PointD>, kShadowBuckets + 1 + kHighlightBuckets> buckets;

  model.ReadTriangles(tileRect, m_zoomLevel, [&](terrain::Triangles const & feature)
  {
    if (CheckCancelled())
      return;
    for (size_t t = 0; t + 2 < feature.m_triangles.size(); t += 3)
    {
      uint32_t const i0 = feature.m_triangles[t];
      uint32_t const i1 = feature.m_triangles[t + 1];
      uint32_t const i2 = feature.m_triangles[t + 2];
      m2::PointD const & p0 = feature.m_points[i0];
      m2::PointD const & p1 = feature.m_points[i1];
      m2::PointD const & p2 = feature.m_points[i2];

      // The features overhang the tile: cheap-reject the outside triangles.
      m2::RectD triRect(p0, p1);
      triRect.Add(p2);
      if (!tileRect.IsIntersect(triRect))
        continue;

      // The upward normal of the CCW triangle, altitudes scaled into mercator units.
      double const e1x = p1.x - p0.x, e1y = p1.y - p0.y;
      double const e2x = p2.x - p0.x, e2y = p2.y - p0.y;
      double const e1z = (feature.m_altitudes[i1] - feature.m_altitudes[i0]) * zScale;
      double const e2z = (feature.m_altitudes[i2] - feature.m_altitudes[i0]) * zScale;
      double const nx = e1y * e2z - e1z * e2y;
      double const ny = e1z * e2x - e1x * e2z;
      double const nz = e1x * e2y - e1y * e2x;
      double const len = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (len < 1e-15)
        continue;
      double const intensity = (nx * kLightX + ny * kLightY + nz * kLightZ) / len;

      int bucket;
      if (intensity < kFlatIntensity)
        bucket = -static_cast<int>(
            std::lround((kFlatIntensity - std::max(intensity, 0.0)) / kFlatIntensity * kShadowBuckets));
      else
        bucket =
            static_cast<int>(std::lround((intensity - kFlatIntensity) / (1.0 - kFlatIntensity) * kHighlightBuckets));
      if (bucket == 0)
        continue;

      auto & out = buckets[bucket + kShadowBuckets];
      auto const emit = [&out](m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
      {
        out.push_back(a);
        out.push_back(b);
        out.push_back(c);
      };
      // Clip to the tile: the alpha layer must not double-blend across the tiles. The
      // drape areas use the CW winding (cf. ApplyAreaFeature), TWM triangles are CCW.
      double const orientation = m2::robust::OrientedS(p0, p1, p2);
      double constexpr kEmptyTriangleS = kMwmPointAccuracy * kMwmPointAccuracy * 0.01;
      if (fabs(orientation) < kEmptyTriangleS)
        continue;
      if (orientation < 0)
        m2::ClipTriangleByRect(tileRect, p0, p1, p2, emit);
      else
        m2::ClipTriangleByRect(tileRect, p0, p2, p1, emit);
    }
  });

  if (CheckCancelled())
    return;

  for (size_t i = 0; i < buckets.size(); ++i)
  {
    if (buckets[i].empty())
      continue;

    int const bucket = static_cast<int>(i) - kShadowBuckets;
    dp::Color const color =
        bucket < 0 ? dp::Color(0, 0, 0, static_cast<uint8_t>(kShadowMaxAlpha * -bucket / kShadowBuckets))
                   : dp::Color(255, 255, 255, static_cast<uint8_t>(kHighlightMaxAlpha * bucket / kHighlightBuckets));

    AreaViewParams params;
    params.m_tileCenter = tileRect.Center();
    // Above all the background fills, below the foreground lines; the semi-transparent
    // color picks the TransparentArea program, which renders after the opaque geometry.
    params.m_depth = drule::kMinLayeredDepthFg;
    params.m_color = color;
    params.m_baseGtoPScale = m_applyParams.m_currentScaleGtoP;
    m_applyParams.m_insertShape(make_unique_dp<AreaShape>(std::move(buckets[i]), BuildingOutline{}, params));
  }

  for (auto const & shape : m_mapShapes[df::GeometryType])
    shape->Prepare(m_context->GetTextureManager());

  if (!m_mapShapes[df::GeometryType].empty())
  {
    TMapShapes geomShapes;
    geomShapes.swap(m_mapShapes[df::GeometryType]);
    m_context->Flush(std::move(geomShapes));
  }
}

void RuleDrawer::DrawDynamicIsolines(MapDataProvider const & model)
{
  ASSERT(m_drawDynamicIsolines, ());
  if (CheckCancelled())
    return;

  // Resolve the isoline line drules for this zoom, one per step class. The altitude ->
  // class mapping matches the baked isoline features, see generator/isolines_generator.cpp.
  static std::array<std::pair<int, char const *>, 5> const kAltClasses = {
      {{1000, "step_1000"}, {500, "step_500"}, {100, "step_100"}, {50, "step_50"}, {10, "step_10"}}};

  auto const resolveLineRule = [this](char const * subType) -> drule::LineRule const *
  {
    auto const & cl = classif();
    uint32_t const type = cl.GetTypeByPath({"isoline", subType});
    drule::KeysT keys;
    cl.GetObject(type)->GetSuitable(m_zoomLevel, feature::GeomType::Line, keys);
    for (auto const & key : keys)
      if (key.m_type == drule::line)
        return drule::GetCurrentRules().Find(key)->GetLine();
    return nullptr;  // Invisible at this zoom.
  };

  drule::LineRule const * const zeroRule = resolveLineRule("zero");
  std::array<drule::LineRule const *, kAltClasses.size()> stepRules;
  for (size_t i = 0; i < kAltClasses.size(); ++i)
    stepRules[i] = resolveLineRule(kAltClasses[i].second);

  auto const ruleForAltitude = [&](int altitude) -> drule::LineRule const *
  {
    if (altitude == 0)
      return zeroRule;
    for (size_t i = 0; i < kAltClasses.size(); ++i)
      if (altitude % kAltClasses[i].first == 0)
        return stepRules[i];
    return nullptr;
  };

  double const visScale = m_applyParams.m_vparams.GetVisualScale();
  ClipSplinesBuilder builder(m_applyParams);

  // The inflated rect keeps the smoothing control points beyond the tile edge, so the
  // smoothed curves continue seamlessly across tiles (see ClipSplinesBuilder::Release).
  m2::RectD queryRect = m_applyParams.m_tileRect;
  queryRect.Scale(kIsolineSmoothScale);

  // The altitude labels policy needs the tile relief upfront: buffer the drawn isolines.
  std::vector<terrain::Isoline> isolines;
  geometry::Altitude minAltitude = std::numeric_limits<geometry::Altitude>::max();
  geometry::Altitude maxAltitude = std::numeric_limits<geometry::Altitude>::min();
  model.ReadIsolines(queryRect, m_zoomLevel, [&](terrain::Isoline && isoline)
  {
    if (CheckCancelled() || ruleForAltitude(isoline.m_altitude) == nullptr)
      return;
    minAltitude = std::min(minAltitude, isoline.m_altitude);
    maxAltitude = std::max(maxAltitude, isoline.m_altitude);
    isolines.push_back(std::move(isoline));
  });
  if (CheckCancelled() || isolines.empty())
    return;

  // Labels ride the isoline pathtext drules (their font and priority are zoom-constant in
  // the styles), but the density is dynamic: terrain::GetIsolinesLabelStepForZoom picks the
  // labeled levels from the tile relief, overriding the per-class style zoom gates - a class
  // whose pathtext is gated to a deeper zoom resolves at the upper style scale instead.
  auto const labelStep =
      terrain::GetIsolinesLabelStepForZoom(m_zoomLevel, static_cast<geometry::Altitude>(maxAltitude - minAltitude));

  auto const resolvePathtextRule = [this](char const * subType) -> drule::PathTextRule const *
  {
    auto const & cl = classif();
    uint32_t const type = cl.GetTypeByPath({"isoline", subType});
    for (int const zoom : {static_cast<int>(m_zoomLevel), scales::GetUpperStyleScale()})
    {
      drule::KeysT keys;
      cl.GetObject(type)->GetSuitable(zoom, feature::GeomType::Line, keys);
      for (auto const & key : keys)
      {
        if (key.m_type != drule::pathtext)
          continue;
        auto const * rule = drule::GetCurrentRules().Find(key)->GetPathtext();
        if (rule != nullptr && rule->primary)
          return rule;
      }
    }
    return nullptr;  // No isoline labels in this style.
  };

  drule::PathTextRule const * zeroTextRule = nullptr;
  std::array<drule::PathTextRule const *, kAltClasses.size()> stepTextRules{};
  if (labelStep != 0)
  {
    zeroTextRule = resolvePathtextRule("zero");
    for (size_t i = 0; i < kAltClasses.size(); ++i)
      stepTextRules[i] = resolvePathtextRule(kAltClasses[i].second);
  }

  auto const textRuleForAltitude = [&](int altitude) -> drule::PathTextRule const *
  {
    if (labelStep == 0 || altitude % labelStep != 0)
      return nullptr;
    if (altitude == 0)
      return zeroTextRule;
    for (size_t i = 0; i < kAltClasses.size(); ++i)
      if (altitude % kAltClasses[i].first == 0)
        return stepTextRules[i];
    return nullptr;
  };

  // All the dynamic labels of a tile share an invalid FeatureID, so their OverlayID
  // uniqueness rests entirely on the running text index (cf. kPathTextBaseTextIndex
  // of the baked path texts); each shape reserves one index per repeated placement.
  uint32_t textIndex = 128;

  for (auto & isoline : isolines)
  {
    if (CheckCancelled())
      return;
    auto const * lineRule = ruleForAltitude(isoline.m_altitude);
    ASSERT(lineRule != nullptr, ());
    auto const * textRule = textRuleForAltitude(isoline.m_altitude);

    builder.SetPath(std::move(isoline.m_points));
    for (auto const & spline : builder.Release(true /* isIsoline */))
    {
      LineViewParams params;
      params.m_tileCenter = m_applyParams.m_tileRect.Center();
      ExtractLineParams(*lineRule, visScale, params);
      // TEST MODE: depth contours (negative altitudes, the bathymetry experiment) go blue,
      // keeping the width and the class alpha of the land isoline drules.
      if (isoline.m_altitude < 0)
        params.m_color = dp::Color(38, 118, 196, params.m_color.GetAlpha());
      // Isoline drules are FG lines, their priorities map to the depth directly.
      params.m_depth = lineRule->priority;
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
      textParams.m_mainText = strings::to_string(isoline.m_altitude);
      float constexpr kMinVisibleFontSize = 8.0f;
      textParams.m_textFont = dp::FontDecl(
          ToDrapeColor(caption.color), std::max(kMinVisibleFontSize, static_cast<float>(caption.height * visScale)));
      // TEST MODE: depth labels match the blue depth contours of the bathymetry experiment.
      if (isoline.m_altitude < 0)
        textParams.m_textFont.m_color = dp::Color(25, 96, 168, 255);
      if (caption.stroke_color != 0)
        textParams.m_textFont.m_outlineColor = ToDrapeColor(caption.stroke_color);
      textParams.m_baseGtoPScale = m_applyParams.m_currentScaleGtoP;

      auto shape = make_unique_dp<PathTextShape>(spline, textParams, m_applyParams.m_tileKey, textIndex);
      // The layout fails on splines too short for the text: no label then.
      if (!shape->CalculateLayout(m_context->GetTextureManager()))
        continue;
      textIndex += static_cast<uint32_t>(shape->GetOffsets().size());
      m_applyParams.m_insertShape(std::move(shape));
    }
  }

  for (auto const & shape : m_mapShapes[df::GeometryType])
    shape->Prepare(m_context->GetTextureManager());

  if (!m_mapShapes[df::GeometryType].empty())
  {
    TMapShapes geomShapes;
    geomShapes.swap(m_mapShapes[df::GeometryType]);
    m_context->Flush(std::move(geomShapes));
  }
}

#ifdef DRAW_TILE_NET
void RuleDrawer::DrawTileNet()
{
  if (CheckCancelled())
    return;

  auto const key = m_context->GetTileKey();

  std::vector<m2::PointD> path;
  path.reserve(4);
  path.push_back(m_globalRect.LeftBottom());
  path.push_back(m_globalRect.LeftTop());
  path.push_back(m_globalRect.RightTop());
  path.push_back(m_globalRect.RightBottom());

  m2::SharedSpline spline(path);
  df::LineViewParams p;
  p.m_tileCenter = m_globalRect.Center();
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
  tp.m_tileCenter = m_globalRect.Center();
  tp.m_titleDecl.m_anchor = dp::Center;
  tp.m_depth = dp::kMaxDepth;
  tp.m_depthLayer = DepthLayer::OverlayLayer;
  tp.m_titleDecl.m_primaryText = key.Coord2String();

  tp.m_titleDecl.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 30);
  tp.m_titleDecl.m_primaryOffset = {0.0f, 0.0f};
  auto textShape =
      make_unique_dp<TextShape>(m_globalRect.Center(), tp, key, m2::PointF(0.0f, 0.0f) /* symbolSize */,
                                m2::PointF(0.0f, 0.0f) /* symbolOffset */, dp::Anchor::Center, 0 /* textIndex */);
  textShape->DisableDisplacing();

  textShape->Prepare(m_context->GetTextureManager());
  TMapShapes overlayShapes;
  overlayShapes.push_back(std::move(textShape));
  m_context->FlushOverlays(std::move(overlayShapes));
}
#endif
}  // namespace df
