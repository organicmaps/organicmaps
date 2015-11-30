#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"

#include "base/assert.hpp"
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

size_t kMinFlushSizes[df::PrioritiesCount] =
{
  1, // AreaPriority
  5, // TextAndPoiPriority
  10, // LinePriority
};

RuleDrawer::RuleDrawer(TDrawerCallback const & fn, ref_ptr<EngineContext> context)
  : m_callback(fn)
  , m_context(context)
{
  m_globalRect = m_context->GetTileKey().GetGlobalRect();

  int32_t tileSize = df::VisualParams::Instance().GetTileSize();
  m_geometryConvertor.OnSize(0, 0, tileSize, tileSize);
  m_geometryConvertor.SetFromRect(m2::AnyRectD(m_globalRect));
  m_currentScaleGtoP = 1.0f / m_geometryConvertor.GetScale();

  for (size_t i = 0; i < m_mapShapes.size(); i++)
    m_mapShapes[i].reserve(kMinFlushSizes[i] + 1);
}

RuleDrawer::~RuleDrawer()
{
  for (auto & shapes : m_mapShapes)
  {
    if (shapes.empty())
      continue;

    for (auto const & shape : shapes)
      shape->Prepare(m_context->GetTextureManager());

    m_context->Flush(move(shapes));
  }
}

void RuleDrawer::operator()(FeatureType const & f)
{
  Stylist s;
  m_callback(f, s);

  if (s.IsEmpty())
    return;

  if (s.IsCoastLine() && (!m_coastlines.insert(s.GetCaptionDescription().GetMainText()).second))
    return;

#ifdef DEBUG
  // Validate on feature styles
  if (s.AreaStyleExists() == false)
  {
    int checkFlag = s.PointStyleExists() ? 1 : 0;
    checkFlag += s.LineStyleExists() ? 1 : 0;
    ASSERT(checkFlag == 1, ());
  }
#endif

  int const zoomLevel = m_context->GetTileKey().m_zoomLevel;
  int const minVisibleScale = feature::GetMinDrawableScale(f);

  auto insertShape = [this](drape_ptr<MapShape> && shape)
  {
    int const index = static_cast<int>(shape->GetPriority());
    ASSERT_LESS(index, m_mapShapes.size(), ());
    m_mapShapes[index].push_back(move(shape));
  };

  if (s.AreaStyleExists())
  {
    ApplyAreaFeature apply(insertShape, f.GetID(), minVisibleScale, f.GetRank(), s.GetCaptionDescription());
    f.ForEachTriangleRef(apply, zoomLevel);

    if (s.PointStyleExists())
      apply(feature::GetCenter(f, zoomLevel));

    s.ForEachRule(bind(&ApplyAreaFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }
  else if (s.LineStyleExists())
  {
    ApplyLineFeature apply(insertShape, f.GetID(), minVisibleScale, f.GetRank(),
                           s.GetCaptionDescription(), m_currentScaleGtoP,
                           zoomLevel >= kLineSimplifyLevelStart && zoomLevel <= kLineSimplifyLevelEnd,
                           f.GetPointsCount());
    f.ForEachPointRef(apply, zoomLevel);

    if (apply.HasGeometry())
      s.ForEachRule(bind(&ApplyLineFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }
  else
  {
    ASSERT(s.PointStyleExists(), ());
    ApplyPointFeature apply(insertShape, f.GetID(), minVisibleScale, f.GetRank(), s.GetCaptionDescription());
    f.ForEachPointRef(apply, zoomLevel);

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
  tp.m_depth = 0;
  tp.m_primaryText = strings::to_string(key.m_x) + " " +
                     strings::to_string(key.m_y) + " " +
                     strings::to_string(key.m_zoomLevel);

  tp.m_primaryTextFont = dp::FontDecl(dp::Color::Red(), 30);

  insertShape(make_unique_dp<TextShape>(r.Center(), tp));
#endif

  for (size_t i = 0; i < m_mapShapes.size(); i++)
  {
    if (m_mapShapes[i].size() < kMinFlushSizes[i])
      continue;

    for (auto const & shape : m_mapShapes[i])
      shape->Prepare(m_context->GetTextureManager());

    TMapShapes mapShapes;
    mapShapes.swap(m_mapShapes[i]);
    m_context->Flush(move(mapShapes));
  }
}

} // namespace df
