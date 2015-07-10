#include "drape_frontend/rule_drawer.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"

#include "base/assert.hpp"
#include "std/bind.hpp"

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

  m_context->Flush(move(m_mapShapes));
}

} // namespace df
