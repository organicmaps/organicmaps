#include "rule_drawer.hpp"
#include "stylist.hpp"
#include "shape_view_params.hpp"
#include "engine_context.hpp"
#include "vizualization_params.hpp"

#include "line_shape.hpp"
#include "area_shape.hpp"

#include "../indexer/drules_include.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/feature_algo.hpp"
#include "../indexer/drawing_rules.hpp"

#include "../map/geometry_processors.hpp"

#include "../base/assert.hpp"

#include "../std/vector.hpp"
#include "../std/bind.hpp"

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
      case ::NOJOIN    : params.m_join = df::NonJoin;
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

  // ============================================= //

  namespace
  {
    class TrianglesFunctor
    {
    public:
      TrianglesFunctor(ScreenBase const & convertor, vector<m2::PointF> & triangles)
        : m_convertor(convertor)
        , m_triangles(triangles)
      {
      }

      void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
      {
        m2::PointF points[3] = { m_convertor.GtoP(p1), m_convertor.GtoP(p2), m_convertor.GtoP(p3) };
        m2::RectF r(points[0], points[1]);
        r.Add(points[2]);

        double const eps = 1.0;
        if (r.SizeX() < eps && r.SizeY() < 1.0)
          return;

        m_triangles.push_back(points[0]);
        m_triangles.push_back(points[1]);
        m_triangles.push_back(points[2]);
      }

    private:
      ScreenBase const & m_convertor;
      vector<m2::PointF> & m_triangles;
    };

    class ApplyAreaFeature
    {
    public:
      ApplyAreaFeature(EngineContext & context,
                       TileKey tileKey,
                       vector<m2::PointF> & triangles)
        : m_context(context)
        , m_tileKey(tileKey)
        , m_triangles(triangles)
        , m_hasCenter(false)
      {
      }

      void SetCenter(m2::PointF const & center) { m_center = center; }

      void ProcessRule(Stylist::rule_wrapper_t const & rule)
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
        {
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
      }

      void Finish()
      {
        if (!m_hasCenter)
          return;

        // TODO
        // create symbol or circle of symbol with circle
      }

    private:
      EngineContext & m_context;
      TileKey m_tileKey;
      vector<m2::PointF> m_triangles;
      bool m_hasCenter;
      m2::PointF m_center;

      double m_symbolDepth;
      double m_circleDepth;
      CircleRuleProto const * m_circleRule;
      SymbolRuleProto const * m_symbolRule;
    };
  }

  // ==================================================== //

  RuleDrawer::RuleDrawer(drawer_callback_fn const & fn, const TileKey & tileKey, EngineContext & context)
    : m_callback(fn)
    , m_tileKey(tileKey)
    , m_context(context)
  {
    m_globalRect = m_tileKey.GetGlobalRect();

    int32_t tileSize = m_context.GetScalesProcessor().GetTileSize();
    m_geometryConvertor.OnSize(0, 0, tileSize, tileSize);
    m_geometryConvertor.SetFromRect(m2::AnyRectD(m_globalRect));
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

    if (s.AreaStyleExists())
    {
      vector<m2::PointF> triangles;

      TrianglesFunctor fun(m_geometryConvertor, triangles);
      f.ForEachTriangleRef(fun, m_tileKey.m_zoomLevel);

      ApplyAreaFeature apply(m_context, m_tileKey, triangles);
      if (s.PointStyleExists())
        apply.SetCenter(feature::GetCenter(f, m_tileKey.m_zoomLevel));

      s.ForEachRule(bind(&ApplyAreaFeature::ProcessRule, &apply, _1));
      apply.Finish();
    }
  }
}
