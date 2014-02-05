#include "apply_feature_functors.hpp"
#include "shape_view_params.hpp"
#include "vizualization_params.hpp"
#include "engine_context.hpp"

#include "area_shape.hpp"
#include "line_shape.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/drules_include.hpp"

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
      params.m_width = max(lineRule->width() * df::VizualizationParams::GetVisualScale(), 1.0);

      switch(lineRule->cap())
      {
      case ::ROUNDCAP : params.m_cap = df::RoundCap;
        break;
      case ::BUTTCAP  : params.m_cap = df::ButtCap;
        break;
      case ::SQUARECAP: params.m_cap = df::SquareCap;
        break;
      default:
        ASSERT(false, ());
      }

      switch (lineRule->join())
      {
      case ::NOJOIN    : params.m_join = df::MiterJoin;
        break;
      case ::ROUNDJOIN : params.m_join = df::RoundJoin;
        break;
      case ::BEVELJOIN : params.m_join = df::BevelJoin;
        break;
      default:
        ASSERT(false, ());
      }
    }
  }

  BaseApplyFeature::BaseApplyFeature(EngineContext & context, TileKey tileKey)
    : m_context(context)
    , m_tileKey(tileKey)
  {
  }

  // ============================================= //

  ApplyPointFeature::ApplyPointFeature(EngineContext & context, TileKey tileKey)
    : base_t(context, tileKey)
    , m_hasPoint(false)
    , m_symbolDepth(graphics::minDepth)
    , m_circleDepth(graphics::minDepth)
    , m_symbolRule(NULL)
    , m_circleRule(NULL)
  {
  }

  void ApplyPointFeature::operator()(const CoordPointT & point)
  {
    operator()(m2::PointF(point.first, point.second));
  }

  void ApplyPointFeature::operator()(m2::PointD const & point)
  {
    m_hasPoint = true;
    m_centerPoint = point;
  }

  void ApplyPointFeature::ProcessRule(const Stylist::rule_wrapper_t & rule)
  {
    if (m_hasPoint == false)
      return;
    drule::BaseRule const * pRule = rule.first;
    float depth = rule.second;

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
      // draw circle
    }
    else if (m_symbolRule)
    {
      // draw symbol
    }
  }

  // ============================================= //

  ApplyAreaFeature::ApplyAreaFeature(EngineContext & context, TileKey tileKey)
    : base_t(context, tileKey)
  {
  }

  void ApplyAreaFeature::operator()(const m2::PointD & p1, const m2::PointD & p2, const m2::PointD & p3)
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
      AreaShape * shape = new AreaShape(ToDrapeColor(areaRule->color()), depth);
      for (size_t i = 0; i < m_triangles.size(); i += 3)
        shape->AddTriangle(m_triangles[i], m_triangles[i + 1], m_triangles[i + 2]);

      m_context.InsertShape(m_tileKey, MovePointer<MapShape>(shape));
    }
    else
      base_t::ProcessRule(rule);
  }

  // ============================================= //

  ApplyLineFeature::ApplyLineFeature(EngineContext & context, TileKey tileKey)
    : base_t(context, tileKey)
  {
  }

  void ApplyLineFeature::operator ()(const CoordPointT & point)
  {
    m_path.push_back(m2::PointF(point.first, point.second));
  }

  void ApplyLineFeature::ProcessRule(const Stylist::rule_wrapper_t & rule)
  {
    LineDefProto const * pRule = rule.first->GetLine();
    float depth = rule.second;

    if (pRule != NULL)
    {
      LineViewParams params;
      Extract(pRule, params);

      m_context.InsertShape(m_tileKey, MovePointer<MapShape>(new LineShape(m_path, depth, params)));
    }
  }
}
