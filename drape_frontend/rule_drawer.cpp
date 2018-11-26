#include "drape_frontend/rule_drawer.hpp"

#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/traffic_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/drape_diagnostics.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/ftypes_sponsored.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/road_shields_parser.hpp"
#include "indexer/scales.hpp"

#include "geometry/clipping.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "drape_frontend/text_shape.hpp"
#ifdef DRAW_TILE_NET
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/text_shape.hpp"

#include "base/string_utils.hpp"
#endif

#include <functional>
#include <utility>
#include <vector>

using namespace std::placeholders;

namespace
{
// The first zoom level in kAverageSegmentsCount.
int constexpr kFirstZoomInAverageSegments = 10;
std::vector<size_t> const kAverageSegmentsCount =
{
  // 10  11    12     13    14    15    16    17    18   19
  10000, 5000, 10000, 5000, 2500, 5000, 2000, 1000, 500, 500
};

double GetBuildingHeightInMeters(FeatureType & f)
{
  double constexpr kDefaultHeightInMeters = 3.0;
  double constexpr kMetersPerLevel = 3.0;

  feature::Metadata const & md = f.GetMetadata();
  double heightInMeters = kDefaultHeightInMeters;

  std::string value = md.Get(feature::Metadata::FMD_HEIGHT);
  if (!value.empty())
  {
    if (!strings::to_double(value, heightInMeters))
      heightInMeters = kDefaultHeightInMeters;
  }
  else
  {
    value = md.Get(feature::Metadata::FMD_BUILDING_LEVELS);
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
  feature::Metadata const & md = f.GetMetadata();
  std::string value = md.Get(feature::Metadata::FMD_MIN_HEIGHT);
  if (value.empty())
    return 0.0;

  double minHeightInMeters;
  if (!strings::to_double(value, minHeightInMeters))
    minHeightInMeters = 0.0;

  return minHeightInMeters;
}

df::BaseApplyFeature::HotelData ExtractHotelData(FeatureType & f)
{
  df::BaseApplyFeature::HotelData result;
  if (ftypes::IsBookingChecker::Instance()(f))
  {
    auto const & metadata = f.GetMetadata();
    result.m_isHotel = true;
    result.m_rating = metadata.Get(feature::Metadata::FMD_RATING);
    if (!strings::to_int(metadata.Get(feature::Metadata::FMD_STARS), result.m_stars))
      result.m_stars = 0;
    if (!strings::to_int(metadata.Get(feature::Metadata::FMD_PRICE_RATE), result.m_priceCategory))
      result.m_priceCategory = 0;
  }
  return result;
}

void ExtractTrafficGeometry(FeatureType const & f, df::RoadClass const & roadClass,
                            m2::PolylineD const & polyline, bool oneWay, int zoomLevel,
                            double pixelToGlobalScale, df::TrafficSegmentsGeometry & geometry)
{
  if (polyline.GetSize() < 2)
    return;

  auto const & regionData = f.GetID().m_mwmId.GetInfo()->GetRegionData();
  bool const isLeftHandTraffic = regionData.Get(feature::RegionData::RD_DRIVING) == "l";

  // Calculate offset between road lines in mercator for two-way roads.
  double twoWayOffset = 0.0;
  if (!oneWay)
    twoWayOffset = pixelToGlobalScale * df::TrafficRenderer::GetTwoWayOffset(roadClass, zoomLevel);

  static std::vector<uint8_t> directions = {traffic::TrafficInfo::RoadSegmentId::kForwardDirection,
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
        m2::PointD const normal = isLeftHandTraffic ? m2::PointD(-tangent.y, tangent.x) :
                                                      m2::PointD(tangent.y, -tangent.x);
        m2::PolylineD segmentPolyline(std::vector<m2::PointD>{segment[0] + normal * twoWayOffset,
                                                              segment[1] + normal * twoWayOffset});
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

bool UsePreciseFeatureCenter(FeatureType & f)
{
  // Add here types for which we want to calculate precise feature center (by best geometry).
  // Warning! Large amount of such objects can reduce performance.
  UNUSED_VALUE(f);
  return false;
}
}  // namespace

namespace df
{
RuleDrawer::RuleDrawer(TDrawerCallback const & drawerFn,
                       TCheckCancelledCallback const & checkCancelled,
                       TIsCountryLoadedByNameFn const & isLoadedFn,
                       TFilterFeatureFn const & filterFn,
                       ref_ptr<EngineContext> engineContext)
  : m_callback(drawerFn)
  , m_checkCancelled(checkCancelled)
  , m_isLoadedFn(isLoadedFn)
  , m_filter(filterFn)
  , m_context(engineContext)
  , m_customFeaturesContext(engineContext->GetCustomFeaturesContext().lock())
  , m_wasCancelled(false)
{
  ASSERT(m_callback != nullptr, ());
  ASSERT(m_checkCancelled != nullptr, ());
  ASSERT(m_filter != nullptr, ());

  m_globalRect = m_context->GetTileKey().GetGlobalRect();

  auto & vparams = df::VisualParams::Instance();
  int32_t tileSize = vparams.GetTileSize();
  m2::RectD const r = m_context->GetTileKey().GetGlobalRect(false /* clipByDataMaxZoom */);
  ScreenBase geometryConvertor;
  geometryConvertor.OnSize(0, 0, tileSize, tileSize);
  geometryConvertor.SetFromRect(m2::AnyRectD(r));
  m_currentScaleGtoP = 1.0f / geometryConvertor.GetScale();

  // Here we support only two virtual tile size: 2048 px for high resolution and 1024 px for others.
  // It helps to render traffic the same on wide range of devices.
  uint32_t const kTrafficTileSize = vparams.GetVisualScale() < df::VisualParams::kXxhdpiScale ? 1024 : 2048;
  geometryConvertor.OnSize(0, 0, kTrafficTileSize, kTrafficTileSize);
  geometryConvertor.SetFromRect(m2::AnyRectD(r));
  m_trafficScalePtoG = geometryConvertor.GetScale();

  int const kAverageOverlaysCount = 200;
  m_mapShapes[df::OverlayType].reserve(kAverageOverlaysCount);
}

RuleDrawer::~RuleDrawer()
{
  if (m_wasCancelled)
    return;

  for (auto const & shape : m_mapShapes[df::OverlayType])
    shape->Prepare(m_context->GetTextureManager());

  if (!m_mapShapes[df::OverlayType].empty())
  {
    TMapShapes overlayShapes;
    overlayShapes.swap(m_mapShapes[df::OverlayType]);
    m_context->FlushOverlays(std::move(overlayShapes));
  }

  m_context->FlushTrafficGeometry(std::move(m_trafficGeometry));
}

bool RuleDrawer::CheckCancelled()
{
  m_wasCancelled = m_checkCancelled();
  return m_wasCancelled;
}

bool RuleDrawer::CheckCoastlines(FeatureType & f, Stylist const & s)
{
  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;

  if (s.IsCoastLine() &&
      zoomLevel > scales::GetUpperWorldScale() &&
      f.GetID().m_mwmId.GetInfo()->GetType() == MwmInfo::COASTS)
  {
    string name;
    if (f.GetName(StringUtf8Multilang::kDefaultCode, name))
    {
      ASSERT(!name.empty(), ());
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

void RuleDrawer::ProcessAreaStyle(FeatureType & f, Stylist const & s,
                                  TInsertShapeFn const & insertShape, int & minVisibleScale)
{
  bool isBuilding = false;
  bool is3dBuilding = false;
  bool isBuildingOutline = false;
  if (f.GetLayer() >= 0)
  {
    bool const hasParts = IsBuildingHasPartsChecker::Instance()(f);
    bool const isPart = IsBuildingPartChecker::Instance()(f);

    // Looks like nonsense, but there are some osm objects with types
    // highway-path-bridge and building (sic!) at the same time (pedestrian crossing).
    isBuilding = (isPart || ftypes::IsBuildingChecker::Instance()(f)) &&
        !ftypes::IsBridgeChecker::Instance()(f) &&
        !ftypes::IsTunnelChecker::Instance()(f);

    isBuildingOutline = isBuilding && hasParts && !isPart;
    is3dBuilding = m_context->Is3dBuildingsEnabled() && (isBuilding && !isBuildingOutline);
  }

  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;

  m2::PointD featureCenter;

  bool hatchingArea = false;

  float areaHeight = 0.0f;
  float areaMinHeight = 0.0f;
  if (is3dBuilding)
  {
    double const heightInMeters = GetBuildingHeightInMeters(f);
    double const minHeightInMeters = GetBuildingMinHeightInMeters(f);
    featureCenter = feature::GetCenter(f, zoomLevel);
    double const lon = MercatorBounds::XToLon(featureCenter.x);
    double const lat = MercatorBounds::YToLat(featureCenter.y);

    m2::RectD rectMercator = MercatorBounds::MetersToXY(lon, lat, heightInMeters);
    areaHeight = static_cast<float>((rectMercator.SizeX() + rectMercator.SizeY()) * 0.5);

    rectMercator = MercatorBounds::MetersToXY(lon, lat, minHeightInMeters);
    areaMinHeight = static_cast<float>((rectMercator.SizeX() + rectMercator.SizeY()) * 0.5);
  }
  else
  {
    hatchingArea = IsHatchingTerritoryChecker::Instance()(f);
  }

  bool applyPointStyle = s.PointStyleExists();
  if (applyPointStyle)
  {
    if (!is3dBuilding)
      featureCenter = feature::GetCenter(f, zoomLevel);
    applyPointStyle = m_globalRect.IsPointInside(featureCenter);
  }

  if (applyPointStyle || is3dBuilding)
    minVisibleScale = feature::GetMinDrawableScale(f);

  ApplyAreaFeature apply(m_context->GetTileKey(), insertShape, f.GetID(),
                         m_currentScaleGtoP, isBuilding,
                         m_context->Is3dBuildingsEnabled() && isBuildingOutline,
                         areaMinHeight, areaHeight, minVisibleScale, f.GetRank(),
                         s.GetCaptionDescription(), hatchingArea);
  f.ForEachTriangle(apply, zoomLevel);
  apply.SetHotelData(ExtractHotelData(f));
  if (applyPointStyle)
  {
    if (UsePreciseFeatureCenter(f))
    {
      f.ResetGeometry();
      featureCenter = feature::GetCenter(f, FeatureType::BEST_GEOMETRY);
    }
    bool const isUGC = m_context->IsUGC(f.GetID());
    apply(featureCenter, true /* hasArea */, isUGC);
  }

  if (CheckCancelled())
    return;

  s.ForEachRule(std::bind(&ApplyAreaFeature::ProcessAreaRule, &apply, _1));

  if (!m_customFeaturesContext || !m_customFeaturesContext->NeedDiscardGeometry(f.GetID()))
    apply.Finish(m_context->GetTextureManager());
}

void RuleDrawer::ProcessLineStyle(FeatureType & f, Stylist const & s,
                                  TInsertShapeFn const & insertShape, int & minVisibleScale)
{
  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;

  ApplyLineFeatureGeometry applyGeom(m_context->GetTileKey(), insertShape, f.GetID(),
                                     m_currentScaleGtoP, minVisibleScale, f.GetRank(),
                                     f.GetPointsCount());
  f.ForEachPoint(applyGeom, zoomLevel);

  if (CheckCancelled())
    return;

  if (applyGeom.HasGeometry())
    s.ForEachRule(std::bind(&ApplyLineFeatureGeometry::ProcessLineRule, &applyGeom, _1));
  applyGeom.Finish();

  std::vector<m2::SharedSpline> clippedSplines;
  bool needAdditional;
  auto const metalineSpline = m_context->GetMetalineManager()->GetMetaline(f.GetID());
  if (metalineSpline.IsNull())
  {
    // There is no metaline for this feature.
    needAdditional = true;
    clippedSplines = applyGeom.GetClippedSplines();
  }
  else if (m_usedMetalines.find(metalineSpline.Get()) != m_usedMetalines.end())
  {
    // Metaline has been used already, skip additional generation.
    needAdditional = false;
  }
  else
  {
    // Generate additional by metaline, mark metaline spline as used.
    needAdditional = true;
    clippedSplines = m2::ClipSplineByRect(m_context->GetTileKey().GetGlobalRect(),
                                          metalineSpline);
    m_usedMetalines.insert(metalineSpline.Get());
  }

  if (needAdditional && !clippedSplines.empty())
  {
    ApplyLineFeatureAdditional applyAdditional(m_context->GetTileKey(), insertShape, f.GetID(),
                                               m_currentScaleGtoP, minVisibleScale, f.GetRank(),
                                               s.GetCaptionDescription(), clippedSplines);
    s.ForEachRule(std::bind(&ApplyLineFeatureAdditional::ProcessLineRule, &applyAdditional, _1));
    applyAdditional.Finish(m_context->GetTextureManager(), ftypes::GetRoadShields(f),
                           m_generatedRoadShields);
  }

  if (m_context->IsTrafficEnabled() && zoomLevel >= kRoadClass0ZoomLevel)
  {
    struct Checker
    {
      std::vector<ftypes::HighwayClass> m_highwayClasses;
      int m_zoomLevel;
      df::RoadClass m_roadClass;
    };
    static Checker const checkers[] = {
      {{ftypes::HighwayClass::Trunk, ftypes::HighwayClass::Primary},
       kRoadClass0ZoomLevel, df::RoadClass::Class0},
      {{ftypes::HighwayClass::Secondary, ftypes::HighwayClass::Tertiary},
       kRoadClass1ZoomLevel, df::RoadClass::Class1},
      {{ftypes::HighwayClass::LivingStreet, ftypes::HighwayClass::Service},
       kRoadClass2ZoomLevel, df::RoadClass::Class2}
    };

    bool const oneWay = ftypes::IsOneWayChecker::Instance()(f);
    auto const highwayClass = ftypes::GetHighwayClass(feature::TypesHolder(f));
    for (size_t i = 0; i < ARRAY_SIZE(checkers); ++i)
    {
      auto const & classes = checkers[i].m_highwayClasses;
      if (find(classes.begin(), classes.end(), highwayClass) != classes.end() &&
          zoomLevel >= checkers[i].m_zoomLevel)
      {
        std::vector<m2::PointD> points;
        points.reserve(f.GetPointsCount());
        f.ResetGeometry();
        f.ForEachPoint([&points](m2::PointD const & p) { points.emplace_back(p); },
                       FeatureType::BEST_GEOMETRY);
        ExtractTrafficGeometry(f, checkers[i].m_roadClass, m2::PolylineD(points), oneWay,
                               zoomLevel, m_trafficScalePtoG, m_trafficGeometry);
        break;
      }
    }
  }
}

void RuleDrawer::ProcessPointStyle(FeatureType & f, Stylist const & s,
                                   TInsertShapeFn const & insertShape, int & minVisibleScale)
{
  if (m_customFeaturesContext && m_customFeaturesContext->NeedDiscardGeometry(f.GetID()))
    return;

  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;
  bool const isSpeedCamera = ftypes::IsSpeedCamChecker::Instance()(f);
  if (isSpeedCamera && !GetStyleReader().IsCarNavigationStyle())
    return;

  DepthLayer depthLayer = DepthLayer::OverlayLayer;
  if (isSpeedCamera)
    depthLayer = DepthLayer::NavigationLayer;

  minVisibleScale = feature::GetMinDrawableScale(f);
  ApplyPointFeature apply(m_context->GetTileKey(), insertShape, f.GetID(), minVisibleScale, f.GetRank(),
                          s.GetCaptionDescription(), 0.0f /* posZ */, m_context->GetDisplacementMode(),
                          depthLayer);
  apply.SetHotelData(ExtractHotelData(f));
  bool const isUGC = m_context->IsUGC(f.GetID());
  f.ForEachPoint([&apply, isUGC](m2::PointD const & pt) { apply(pt, false /* hasArea */, isUGC); }, zoomLevel);

  if (CheckCancelled())
    return;

  s.ForEachRule(bind(&ApplyPointFeature::ProcessPointRule, &apply, _1));
  apply.Finish(m_context->GetTextureManager());
}

void RuleDrawer::operator()(FeatureType & f)
{
  if (CheckCancelled())
    return;

  if (m_filter(f))
    return;

  Stylist s;
  m_callback(f, s);

  if (s.IsEmpty())
    return;

  if (!CheckCoastlines(f, s))
    return;

  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;

  // FeatureType::GetLimitRect call invokes full geometry reading and decoding.
  // That's why this code follows after all lightweight return options.
  m2::RectD const limitRect = f.GetLimitRect(zoomLevel);
  if (!m_globalRect.IsIntersect(limitRect))
    return;

#ifdef DEBUG
  // Validate on feature styles
  if (!s.AreaStyleExists())
  {
    int checkFlag = s.PointStyleExists() ? 1 : 0;
    checkFlag += s.LineStyleExists() ? 1 : 0;
    ASSERT(checkFlag == 1, ());
  }
#endif

  int minVisibleScale = 0;
  auto insertShape = [this, &minVisibleScale](drape_ptr<MapShape> && shape)
  {
    int const index = static_cast<int>(shape->GetType());
    ASSERT_LESS(index, static_cast<int>(m_mapShapes.size()), ());

    shape->SetFeatureMinZoom(minVisibleScale);
    m_mapShapes[index].push_back(std::move(shape));
  };

  if (s.AreaStyleExists())
  {
    ProcessAreaStyle(f, s, insertShape, minVisibleScale);
  }
  else if (s.LineStyleExists())
  {
    ProcessLineStyle(f, s, insertShape, minVisibleScale);
  }
  else
  {
    ASSERT(s.PointStyleExists(), ());
    ProcessPointStyle(f, s, insertShape, minVisibleScale);
  }

#ifdef DRAW_TILE_NET
  DrawTileNet(insertShape);
#endif

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

#ifdef DRAW_TILE_NET
void RuleDrawer::DrawTileNet(TInsertShapeFn const & insertShape)
{
  TileKey key = m_context->GetTileKey();
  m2::RectD r = key.GetGlobalRect();
  std::vector<m2::PointD> path;
  path.push_back(r.LeftBottom());
  path.push_back(r.LeftTop());
  path.push_back(r.RightTop());
  path.push_back(r.RightBottom());
  path.push_back(r.LeftBottom());

  m2::SharedSpline spline(path);
  df::LineViewParams p;
  p.m_tileCenter = m_globalRect.Center();
  p.m_baseGtoPScale = 1.0;
  p.m_cap = dp::ButtCap;
  p.m_color = dp::Color::Red();
  p.m_depth = 20000;
  p.m_depthLayer = DepthLayer::GeometryLayer;
  p.m_width = 5;
  p.m_join = dp::RoundJoin;

  insertShape(make_unique_dp<LineShape>(spline, p));

  df::TextViewParams tp;
  tp.m_tileCenter = m_globalRect.Center();
  tp.m_titleDecl.m_anchor = dp::Center;
  tp.m_depth = 20000;
  tp.m_depthLayer = DepthLayer::OverlayLayer;
  tp.m_titleDecl.m_primaryText = strings::to_string(key.m_x) + " " +
                                 strings::to_string(key.m_y) + " " +
                                 strings::to_string(key.m_zoomLevel);

  tp.m_titleDecl.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 30);
  tp.m_titleDecl.m_primaryOffset = {0.0f, 0.0f};
  drape_ptr<TextShape> textShape = make_unique_dp<TextShape>(r.Center(), tp, key,
                                                             m2::PointF(0.0f, 0.0f) /* symbolSize */,
                                                             m2::PointF(0.0f, 0.0f) /* symbolOffset */,
                                                             dp::Anchor::Center,
                                                             0 /* textIndex */);
  textShape->DisableDisplacing();
  insertShape(std::move(textShape));
}
#endif
}  // namespace df
