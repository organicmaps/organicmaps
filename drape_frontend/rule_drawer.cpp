#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"

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

int const SIMPLIFY_BOTTOM = 10;
int const SIMPLIFY_TOP = 12;

RuleDrawer::RuleDrawer(TDrawerCallback const & fn, ref_ptr<EngineContext> context)
  : m_callback(fn)
  , m_context(context)
{
  m_globalRect = m_context->GetTileKey().GetGlobalRect();

  int32_t tileSize = df::VisualParams::Instance().GetTileSize();
  m_geometryConvertor.OnSize(0, 0, tileSize, tileSize);
  m_geometryConvertor.SetFromRect(m2::AnyRectD(m_globalRect));
  m_currentScaleGtoP = 1.0f / m_geometryConvertor.GetScale();
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

  int zoomLevel = m_context->GetTileKey().m_zoomLevel;

  auto insertShape = [this](drape_ptr<MapShape> && shape)
  {
    m_mapShapes.push_back(move(shape));
  };

  if (s.AreaStyleExists())
  {
    ApplyAreaFeature apply(insertShape, f.GetID(), s.GetCaptionDescription());
    f.ForEachTriangleRef(apply, zoomLevel);

    if (s.PointStyleExists())
      apply(feature::GetCenter(f, zoomLevel));

    s.ForEachRule(bind(&ApplyAreaFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }
  else if (s.LineStyleExists())
  {
    ApplyLineFeature apply(insertShape, f.GetID(), s.GetCaptionDescription(), m_currentScaleGtoP,
                           zoomLevel >= SIMPLIFY_BOTTOM && zoomLevel <= SIMPLIFY_TOP, f.GetPointsCount());
    f.ForEachPointRef(apply, zoomLevel);

    if (apply.HasGeometry())
      s.ForEachRule(bind(&ApplyLineFeature::ProcessRule, &apply, _1));
    apply.Finish();
  }
  else
  {
    ASSERT(s.PointStyleExists(), ());
    ApplyPointFeature apply(insertShape, f.GetID(), s.GetCaptionDescription());
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

  for (auto & shape : m_mapShapes)
    shape->Prepare(m_context->GetTextureManager());

  m_context->Flush(move(m_mapShapes));
}

} // namespace df
