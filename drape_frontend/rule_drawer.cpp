#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"

//#define DRAW_TILE_NET

#ifdef DRAW_TILE_NET
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/text_shape.hpp"

#include "base/string_utils.hpp"
#endif

namespace df
{

int const kLineSimplifyLevelStart = 10;
int const kLineSimplifyLevelEnd = 12;

RuleDrawer::RuleDrawer(TDrawerCallback const & fn,
                       TCheckCancelledCallback const & checkCancelled,
                       TIsCountryLoadedByNameFn const & isLoadedFn,
                       ref_ptr<EngineContext> context, bool is3dBuildings)
  : m_callback(fn)
  , m_checkCancelled(checkCancelled)
  , m_isLoadedFn(isLoadedFn)
  , m_context(context)
  , m_is3dBuidings(is3dBuildings)
  , m_wasCancelled(false)
{
  ASSERT(m_callback != nullptr, ());
  ASSERT(m_checkCancelled != nullptr, ());

  m_globalRect = m_context->GetTileKey().GetGlobalRect();

  int32_t tileSize = df::VisualParams::Instance().GetTileSize();
  m2::RectD const r = m_context->GetTileKey().GetGlobalRect(false /* clipByDataMaxZoom */);
  ScreenBase geometryConvertor;
  geometryConvertor.OnSize(0, 0, tileSize, tileSize);
  geometryConvertor.SetFromRect(m2::AnyRectD(r));
  m_currentScaleGtoP = 1.0f / geometryConvertor.GetScale();

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
    m_context->FlushOverlays(move(overlayShapes));
  }
}

bool RuleDrawer::CheckCancelled()
{
  m_wasCancelled = m_checkCancelled();
  return m_wasCancelled;
}

void RuleDrawer::operator()(FeatureType const & f)
{
  if (CheckCancelled())
    return;

  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;

  m2::RectD const limitRect = f.GetLimitRect(zoomLevel);

  if (!m_globalRect.IsIntersect(limitRect))
    return;

  Stylist s;
  m_callback(f, s);

  if (s.IsEmpty())
    return;

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
          return;
        ++iter;
      }
    }
  }

#ifdef DEBUG
  // Validate on feature styles
  if (s.AreaStyleExists() == false)
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
    ASSERT_LESS(index, m_mapShapes.size(), ());

    shape->SetFeatureMinZoom(minVisibleScale);
    m_mapShapes[index].push_back(move(shape));
  };

  if (s.AreaStyleExists())
  {
    bool is3dBuilding = false;
    if (m_is3dBuidings && f.GetLayer() >= 0)
    {
      // Looks like nonsense, but there are some osm objects with types
      // highway-path-bridge and building (sic!) at the same time (pedestrian crossing).
      is3dBuilding = (ftypes::IsBuildingChecker::Instance()(f) ||
                      ftypes::IsBuildingPartChecker::Instance()(f)) &&
          !ftypes::IsBridgeChecker::Instance()(f) &&
          !ftypes::IsTunnelChecker::Instance()(f);
    }

    m2::PointD featureCenter;

    float areaHeight = 0.0f;
    float areaMinHeight = 0.0f;
    if (is3dBuilding)
    {
      feature::Metadata const & md = f.GetMetadata();

      constexpr double kDefaultHeightInMeters = 3.0;
      constexpr double kMetersPerLevel = 3.0;
      double heightInMeters = kDefaultHeightInMeters;

      string value = md.Get(feature::Metadata::FMD_HEIGHT);
      if (!value.empty())
      {
        strings::to_double(value, heightInMeters);
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

      value = md.Get(feature::Metadata::FMD_MIN_HEIGHT);
      double minHeigthInMeters = 0.0;
      if (!value.empty())
        strings::to_double(value, minHeigthInMeters);

      featureCenter = feature::GetCenter(f, zoomLevel);
      double const lon = MercatorBounds::XToLon(featureCenter.x);
      double const lat = MercatorBounds::YToLat(featureCenter.y);

      m2::RectD rectMercator = MercatorBounds::MetresToXY(lon, lat, heightInMeters);
      areaHeight = m2::PointD(rectMercator.SizeX(), rectMercator.SizeY()).Length();

      rectMercator = MercatorBounds::MetresToXY(lon, lat, minHeigthInMeters);
      areaMinHeight = m2::PointD(rectMercator.SizeX(), rectMercator.SizeY()).Length();
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

    ApplyAreaFeature apply(insertShape, f.GetID(), m_globalRect, areaMinHeight, areaHeight,
                           minVisibleScale, f.GetRank(), s.GetCaptionDescription());
    f.ForEachTriangle(apply, zoomLevel);

    if (applyPointStyle)
      apply(featureCenter, true /* hasArea */);

    if (CheckCancelled())
      return;

    s.ForEachRule(bind(&ApplyAreaFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }
  else if (s.LineStyleExists())
  {
    ApplyLineFeature apply(insertShape, f.GetID(), m_globalRect, minVisibleScale, f.GetRank(),
                           s.GetCaptionDescription(), m_currentScaleGtoP,
                           zoomLevel >= kLineSimplifyLevelStart && zoomLevel <= kLineSimplifyLevelEnd,
                           f.GetPointsCount());
    f.ForEachPoint(apply, zoomLevel);

    if (CheckCancelled())
      return;

    if (apply.HasGeometry())
      s.ForEachRule(bind(&ApplyLineFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }
  else
  {
    ASSERT(s.PointStyleExists(), ());

    minVisibleScale = feature::GetMinDrawableScale(f);
    ApplyPointFeature apply(insertShape, f.GetID(), minVisibleScale, f.GetRank(), s.GetCaptionDescription(), 0.0f /* posZ */);
    f.ForEachPoint([&apply](m2::PointD const & pt) { apply(pt, false /* hasArea */); }, zoomLevel);

    if (CheckCancelled())
      return;

    s.ForEachRule(bind(&ApplyPointFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }

#ifdef DRAW_TILE_NET
  TileKey key = m_context->GetTileKey();
  m2::RectD r = key.GetGlobalRect();
  vector<m2::PointD> path;
  path.push_back(r.LeftBottom());
  path.push_back(r.LeftTop());
  path.push_back(r.RightTop());
  path.push_back(r.RightBottom());
  path.push_back(r.LeftBottom());

  m2::SharedSpline spline(path);
  df::LineViewParams p;
  p.m_baseGtoPScale = 1.0;
  p.m_cap = dp::ButtCap;
  p.m_color = dp::Color::Red();
  p.m_depth = 20000;
  p.m_width = 5;
  p.m_join = dp::RoundJoin;

  insertShape(make_unique_dp<LineShape>(spline, p));

  df::TextViewParams tp;
  tp.m_anchor = dp::Center;
  tp.m_depth = 20000;
  tp.m_primaryText = strings::to_string(key.m_x) + " " +
                     strings::to_string(key.m_y) + " " +
                     strings::to_string(key.m_zoomLevel);

  tp.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 30);

  drape_ptr<TextShape> textShape = make_unique_dp<TextShape>(r.Center(), tp, false, 0, false);
  textShape->DisableDisplacing();
  insertShape(move(textShape));
#endif

  if (CheckCancelled())
    return;

  for (auto const & shape : m_mapShapes[df::GeometryType])
    shape->Prepare(m_context->GetTextureManager());

  if (!m_mapShapes[df::GeometryType].empty())
  {
    TMapShapes geomShapes;
    geomShapes.swap(m_mapShapes[df::GeometryType]);
    m_context->Flush(move(geomShapes));
  }
}

} // namespace df
