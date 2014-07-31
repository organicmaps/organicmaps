#include "apply_feature_functors.hpp"
#include "shape_view_params.hpp"
#include "visual_params.hpp"
#include "engine_context.hpp"

#include "area_shape.hpp"
#include "line_shape.hpp"
#include "text_shape.hpp"
#include "poi_symbol_shape.hpp"
#include "path_symbol_shape.hpp"
#include "circle_shape.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/drules_include.hpp"
#include "../std/algorithm.hpp"

#include "../drape/color.hpp"

#include "../graphics/defines.hpp"

namespace df
{

namespace
{

Color ToDrapeColor(uint32_t src)
{
  return Extract(src, 255 - (src >> 24));
}

void Extract(::LineDefProto const * lineRule,
             df::LineViewParams & params)
{
  params.m_color = ToDrapeColor(lineRule->color());
  params.m_width = max(lineRule->width() * df::VisualParams::Instance().GetVisualScale(), 1.0);

  switch(lineRule->cap())
  {
  case ::ROUNDCAP : params.m_cap = dp::RoundCap;
    break;
  case ::BUTTCAP  : params.m_cap = dp::ButtCap;
    break;
  case ::SQUARECAP: params.m_cap = dp::SquareCap;
    break;
  default:
    ASSERT(false, ());
  }

  switch (lineRule->join())
  {
  case ::NOJOIN    : params.m_join = dp::MiterJoin;
    break;
  case ::ROUNDJOIN : params.m_join = dp::RoundJoin;
    break;
  case ::BEVELJOIN : params.m_join = dp::BevelJoin;
    break;
  default:
    ASSERT(false, ());
  }
}

void CaptionDefProtoToFontDecl(CaptionDefProto const * capRule, df::FontDecl &params)
{
  params.m_color = ToDrapeColor(capRule->color());
  params.m_size = max(8.0, capRule->height() * df::VisualParams::Instance().GetVisualScale());

  params.m_needOutline = false;
  if (capRule->has_stroke_color())
  {
    params.m_needOutline = true;
    params.m_outlineColor = Color(255, 255, 255, 255);
  }
}

dp::Anchor GetAnchor(CaptionDefProto const * capRule)
{
  if (capRule->has_offset_y())
  {
    if (capRule->offset_y() > 0)
      return dp::Top;
    else
      return dp::Bottom;
  }
  if (capRule->has_offset_x())
  {
    if (capRule->offset_x() > 0)
      return dp::Right;
    else
      return dp::Left;

  }

  return dp::Center;
}

} // namespace

BaseApplyFeature::BaseApplyFeature(EngineContext & context, TileKey tileKey, FeatureID const & id)
  : m_context(context)
  , m_tileKey(tileKey)
  , m_id(id)
{
}

// ============================================= //

ApplyPointFeature::ApplyPointFeature(EngineContext & context, TileKey tileKey, FeatureID const & id)
  : base_t(context, tileKey, id)
  , m_hasPoint(false)
  , m_symbolDepth(graphics::minDepth)
  , m_circleDepth(graphics::minDepth)
  , m_symbolRule(NULL)
  , m_circleRule(NULL)
{
}

void ApplyPointFeature::operator()(CoordPointT const & point)
{
  operator()(m2::PointF(point.first, point.second));
}

void ApplyPointFeature::operator()(m2::PointD const & point)
{
  m_hasPoint = true;
  m_centerPoint = point;
}

void ApplyPointFeature::ProcessRule(Stylist::rule_wrapper_t const & rule)
{
  if (m_hasPoint == false)
    return;
  drule::BaseRule const * pRule = rule.first;
  float depth = rule.second;

  CaptionDefProto const * capRule = pRule->GetCaption(0);
  if (capRule)
  {
    FontDecl decl;
    CaptionDefProtoToFontDecl(capRule, decl);

    TextViewParams params;
    params.m_anchor = GetAnchor(capRule);
    params.m_depth = depth;
    params.m_featureID = m_id;
    params.m_primaryOffset = m2::PointF(0,0);
    params.m_primaryText = m_primaryText;
    params.m_primaryTextFont = decl;
    float scaleFactor = df::VisualParams::Instance().GetVisualScale();
    if (capRule->has_offset_x())
      params.m_primaryOffset.x = capRule->offset_x() * scaleFactor;

    if (capRule->has_offset_y())
      params.m_primaryOffset.y = capRule->offset_y() * scaleFactor;

    CaptionDefProto const * auxCapRule = pRule->GetCaption(1);
    if (auxCapRule)
    {
      FontDecl auxDecl;
      CaptionDefProtoToFontDecl(auxCapRule, auxDecl);

      params.m_secondaryText = m_secondaryText;
      params.m_secondaryTextFont = auxDecl;
    }
    if(!params.m_primaryText.empty() || !params.m_secondaryText.empty())
      m_context.InsertShape(m_tileKey, MovePointer<MapShape>(new TextShape(m_centerPoint, params)));
  }

  SymbolRuleProto const * symRule =  pRule->GetSymbol();
  if (symRule)
  {
    m_symbolDepth = depth;
    m_symbolRule  = symRule;
  }

  CircleRuleProto const * circleRule = pRule->GetCircle();
  if (circleRule)
  {
    m_circleDepth = depth;
    m_circleRule  = circleRule;
  }
}

void ApplyPointFeature::Finish()
{
  if (m_circleRule && m_symbolRule)
  {
    // draw circledSymbol
  }
  else if (m_circleRule)
  {
    CircleViewParams params(m_id);
    params.m_depth = m_circleDepth;
    params.m_color = ToDrapeColor(m_circleRule->color());
    params.m_radius = m_circleRule->radius();

    CircleShape * shape = new CircleShape(m_centerPoint, params);
    m_context.InsertShape(m_tileKey, MovePointer<MapShape>(shape));
  }
  else if (m_symbolRule)
  {
    PoiSymbolViewParams params(m_id);
    params.m_depth = m_symbolDepth;
    params.m_symbolName = m_symbolRule->name();

    PoiSymbolShape * shape = new PoiSymbolShape(m_centerPoint, params);
    m_context.InsertShape(m_tileKey, MovePointer<MapShape>(shape));
  }
}

// ============================================= //

ApplyAreaFeature::ApplyAreaFeature(EngineContext & context, TileKey tileKey, FeatureID const & id)
  : base_t(context, tileKey, id)
{
}

void ApplyAreaFeature::operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
{
  m_triangles.push_back(p1);
  m_triangles.push_back(p2);
  m_triangles.push_back(p3);
}

void ApplyAreaFeature::ProcessRule(Stylist::rule_wrapper_t const & rule)
{
  drule::BaseRule const * pRule = rule.first;
  double const depth = rule.second;

  AreaRuleProto const * areaRule = pRule->GetArea();

  if (areaRule)
  {
    AreaViewParams params;
    params.m_depth = depth;
    params.m_color = ToDrapeColor(areaRule->color());

    AreaShape * shape = new AreaShape(m_triangles, params);
    m_context.InsertShape(m_tileKey, MovePointer<MapShape>(shape));
  }
  else
    base_t::ProcessRule(rule);
}

// ============================================= //

ApplyLineFeature::ApplyLineFeature(EngineContext & context, TileKey tileKey, FeatureID const & id, double nextModelViewScale)
  : base_t(context, tileKey, id), m_nextModelViewScale(nextModelViewScale)
{
}

void ApplyLineFeature::operator ()(CoordPointT const & point)
{
  m2::PointF const inputPt(point.first, point.second);

  /// TODO remove this check when fix generator.
  /// Now we have line objects with zero length segments
  if (m_path.empty())
  {
    m_path.push_back(inputPt);
  }
  else
  {
    if (!(inputPt - m_path.back()).IsAlmostZero())
      m_path.push_back(inputPt);
    else
      LOG(LDEBUG, ("Found seqment with zero lenth (the ended points are same)"));
  }

}

bool ApplyLineFeature::HasGeometry() const
{
  return m_path.size() > 1;
}

void ApplyLineFeature::ProcessRule(Stylist::rule_wrapper_t const & rule)
{
  LineDefProto const * pRule = rule.first->GetLine();
  float depth = rule.second;

  if (pRule != NULL)
  {
    if (pRule->has_pathsym())
    {
      PathSymProto const & symRule = pRule->pathsym();
      PathSymbolViewParams params;
      params.m_depth = depth;
      params.m_symbolName = symRule.name();
      params.m_step = symRule.offset() * df::VisualParams::Instance().GetVisualScale();
      params.m_offset = symRule.step() * df::VisualParams::Instance().GetVisualScale();

      m_context.InsertShape(m_tileKey, MovePointer<MapShape>(new PathSymbolShape(m_path, params, m_nextModelViewScale)));
    }
    else
    {
      LineViewParams params;
      Extract(pRule, params);
      params.m_depth = depth;
      m_context.InsertShape(m_tileKey, MovePointer<MapShape>(new LineShape(m_path, params)));
    }
  }
}

} // namespace df
