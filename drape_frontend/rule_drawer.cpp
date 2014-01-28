#include "rule_drawer.hpp"
#include "stylist.hpp"
#include "shape_view_params.hpp"
#include "engine_context.hpp"

#include "line_shape.hpp"

#include "../indexer/drules_include.hpp"
#include "../indexer/feature.hpp"

#include "../map/geometry_processors.hpp"

#include "../base/assert.hpp"

namespace df
{
  namespace
  {
    Color ToDrapeColor(uint32_t src)
    {
      return Extract(src, 255 - (src >> 24));
    }

    // ==================================================== //
    void Extract(::LineDefProto const * lineRule,
                 double visualScale,
                 df::LineViewParams & params)
    {
      params.m_color = ToDrapeColor(lineRule->color());
      params.m_width = max(lineRule->width() * visualScale, 1.0);

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

    using namespace  gp;
    if (s.AreaStyleExists())
    {
      typedef filter_screenpts_adapter<area_tess_points> functor_t;

      functor_t::params p;
      p.m_convertor = & m_geometryConvertor;
      p.m_rect = & m_globalRect;

      functor_t fun(p);
      f.ForEachTriangleExRef(fun, m_tileKey.m_zoomLevel);
      list<di::AreaInfo> & info = fun.m_points;
    }
  }
}
