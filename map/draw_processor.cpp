#include "draw_processor.hpp"
#include "drawer_yg.hpp"

#include "../platform/languages.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/rect_intersect.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/drawing_rules.hpp"
#include "../indexer/feature_data.hpp"

#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"


namespace get_pts {

using namespace di;

m2::PointD base::g2p(m2::PointD const & pt) const
{
  return m_convertor->GtoP(pt);
}

base_screen::base_screen(params const & p)
: base(p)
{
  m_convertor->GtoP(*p.m_rect, m_rect);
}

void one_point::operator() (CoordPointT const & p)
{
  ASSERT ( !m_exist, ("point feature should have only one point") );

  m2::PointD pt(make_point(p));

  if (m_rect->IsPointInside(pt))
  {
    m_exist = true;
    m_point = convert_point(pt);
  }
  else m_exist = false;
}

void path_points::StartPL(m2::PointD const & pt)
{
  EndPL();

  m_points.push_back(PathInfo(m_length + m_prev.Length(pt)));
  push_point(pt);

  m_newPL = false;
}

void path_points::EndPL()
{
  if (!m_points.empty() && m_points.back().size() < 2)
    m_points.pop_back();
}

void path_points::simple_filtration(m2::PointD const & pt)
{
  if (m_hasPrevPt)
  {
    if (!m2::RectD(m_prev, pt).IsIntersect(m_rect))
      m_newPL = true;
    else
    {
      if (m_newPL)
        StartPL(m_prev);

      push_point(pt);
    }

    m_length += m_prev.Length(pt);
  }
  else
  {
    m_hasPrevPt = true;
    m_length = 0.0;
  }

  m_prev = pt;
}

void path_points::best_filtration(m2::PointD const & pt)
{
  if (m_hasPrevPt)
  {
    m2::PointD prev = m_prev;
    m2::PointD curr = pt;

    double const segLen = curr.Length(prev);

    if ((m_startLength != 0) && (m_endLength != 0))
    {
      if ((m_startLength >= m_length) && (m_startLength < m_length + segLen))
        m_startLength = m_length;

      if ((m_endLength > m_length) && (m_endLength <= m_length + segLen))
        m_endLength = m_length + segLen;
    }

    if ((m_length >= m_startLength) && (m_endLength >= m_length + segLen))
    {
      /// we're in the dead zone. add without clipping
      if (m_newPL)
        StartPL(prev);

      push_point(curr);
    }
    else
    {
      if (!m2::Intersect(m_rect, prev, curr))
        m_newPL = true;
      else
      {
        if (!equal_glb_pts(prev, m_prev))
          m_newPL = true;

        if (m_newPL)
          StartPL(prev);

        push_point(curr);

        if (!equal_glb_pts(curr, pt))
          m_newPL = true;
      }
    }

    m_length += segLen;
  }
  else
  {
    m_hasPrevPt = true;
    m_length = 0.0;
  }

  m_prev = pt;
}

void path_points::operator() (m2::PointD const & p)
{
  // Choose simple (fast) or best (slow) filtration

  //simple_filtration(p);
  best_filtration(p);
}

bool path_points::IsExist()
{
  // finally, assign whole length to every cutted path
  for_each(m_points.begin(), m_points.end(),
    bind(&di::PathInfo::SetFullLength, _1, m_length));

  EndPL();
  return base_type::IsExist();
}

void area_tess_points::StartPrimitive(size_t ptsCount)
{
  m_points.push_back(AreaInfo());
  m_points.back().reserve(ptsCount);
}

void area_tess_points::operator() (m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
{
  push_point(p1);
  push_point(p2);
  push_point(p3);
}

void area_tess_points::EndPrimitive()
{
  if (m_points.back().size() < 3)
    m_points.pop_back();
}

bool area_tess_points::IsExist() const
{
  return !m_points.empty();
}

}

namespace fwork
{
  namespace
  {
    template <class TSrc> void assign_point(di::DrawInfo * p, TSrc & src)
    {
      p->m_point = src.m_point;
    }
    template <class TSrc> void assign_path(di::DrawInfo * p, TSrc & src)
    {
      p->m_pathes.swap(src.m_points);
    }
    template <class TSrc> void assign_area(di::DrawInfo * p, TSrc & src)
    {
      p->m_areas.swap(src.m_points);

      ASSERT ( !p->m_areas.empty(), () );
      p->m_areas.back().SetCenter(src.GetCenter());
    }
  }

  DrawProcessor::DrawProcessor( m2::RectD const & r,
                                ScreenBase const & convertor,
                                shared_ptr<PaintEvent> const & e,
                                int scaleLevel)
    : m_rect(r),
      m_convertor(convertor),
      m_paintEvent(e),
      m_zoom(scaleLevel),
      m_glyphCache(e->drawer()->screen()->glyphCache())
#ifdef PROFILER_DRAWING
      , m_drawCount(0)
#endif
  {
    GetDrawer()->SetScale(m_zoom);
  }

  namespace
  {
    struct less_depth
    {
      bool operator() (di::DrawRule const & r1, di::DrawRule const & r2) const
      {
        return (r1.m_depth < r2.m_depth);
      }
    };

    struct less_key
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        if (r1.m_type == r2.m_type)
        {
          // assume that unique algo leaves the first element (with max priority), others - go away
          return (r1.m_priority > r2.m_priority);
        }
        else
          return (r1.m_type < r2.m_type);
      }
    };

    struct equal_key
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        // many line and area rules - is ok, other rules - one is enough
        if (r1.m_type == drule::line || r1.m_type == drule::area)
          return (r1 == r2);
        else
          return (r1.m_type == r2.m_type);
      }
    };
  }

  void DrawProcessor::PreProcessKeys(vector<drule::Key> & keys) const
  {
    sort(keys.begin(), keys.end(), less_key());
    keys.erase(unique(keys.begin(), keys.end(), equal_key()), keys.end());
  }

#define GET_POINTS(f, for_each_fun, fun, assign_fun)       \
  {                                                        \
    f.for_each_fun(fun, m_zoom);                           \
    if (fun.IsExist())                                     \
    {                                                      \
      isExist = true;                                      \
      assign_fun(ptr.get(), fun);                          \
    }                                                      \
  }

  bool DrawProcessor::operator() (FeatureType const & f)
  {
    if (m_paintEvent->isCancelled())
      throw redraw_operation_cancelled();

    // get drawing rules
    vector<drule::Key> keys;
    string names;       // for debug use only, in release it's empty
    pair<int, bool> type = feature::GetDrawRule(f, m_zoom, keys, names);

    if (keys.empty())
    {
      // Index can pass here invisible features.
      // During indexing, features are placed at first visible scale bucket.
      // At higher scales it can become invisible - it depends on classificator.
      return true;
    }

    if (type.second)
    {
      // Draw coastlines features only once.
      if (!m_coasts.insert(f.GetPreferredDrawableName(0)).second)
        return true;
    }

    // remove duplicating identical drawing keys
    PreProcessKeys(keys);

    // get drawing rules for the m_keys array
    size_t const count = keys.size();
#ifdef PROFILER_DRAWING
    m_drawCount += count;
#endif

    buffer_vector<di::DrawRule, reserve_rules_count> rules;
    rules.resize(count);

    int layer = f.GetLayer();
    bool isTransparent = false;
    if (layer == feature::LAYER_TRANSPARENT_TUNNEL)
    {
      layer = 0;
      isTransparent = true;
    }

    for (size_t i = 0; i < count; ++i)
    {
      int depth = keys[i].m_priority;
      if (layer != 0)
        depth = (layer * drule::layer_base_priority) + (depth % drule::layer_base_priority);

      rules[i] = di::DrawRule(drule::rules().Find(keys[i]), depth, isTransparent);
    }

    sort(rules.begin(), rules.end(), less_depth());

    shared_ptr<di::DrawInfo> ptr(new di::DrawInfo(
      f.GetPreferredDrawableName(languages::GetCurrentPriorities()),
      f.GetRoadNumber(),
      (m_zoom > 5) ? f.GetPopulationDrawRank() : 0.0));

    DrawerYG * pDrawer = GetDrawer();

    using namespace get_pts;

    bool isExist = false;
    switch (type.first)
    {
    case feature::GEOM_POINT:
    {
      typedef get_pts::one_point functor_t;

      functor_t::params p;
      p.m_convertor = &m_convertor;
      p.m_rect = &m_rect;

      functor_t fun(p);
      GET_POINTS(f, ForEachPointRef, fun, assign_point)
      break;
    }

    case feature::GEOM_AREA:
    {
      typedef filter_screenpts_adapter<area_tess_points> functor_t;

      functor_t::params p;
      p.m_convertor = &m_convertor;
      p.m_rect = &m_rect;

      functor_t fun(p);
      GET_POINTS(f, ForEachTriangleExRef, fun, assign_area)
      {
        // if area feature has any line-drawing-rules, than draw it like line
        for (size_t i = 0; i < keys.size(); ++i)
          if (keys[i].m_type == drule::line)
            goto draw_line;
        break;
      }
    }
    draw_line:
    case feature::GEOM_LINE:
      {
        typedef filter_screenpts_adapter<path_points> functor_t;
        functor_t::params p;
        p.m_convertor = &m_convertor;
        p.m_rect = &m_rect;

        if (!ptr->m_name.empty())
        {
          double fontSize = 0;
          for (size_t i = 0; i < count; ++i)
          {
            if (pDrawer->filter_text_size(rules[i].m_rule))
              fontSize = max((uint8_t)fontSize, pDrawer->get_pathtext_font_size(rules[i].m_rule));
          }

          if (fontSize != 0)
          {
            double textLength = m_glyphCache->getTextLength(fontSize, ptr->m_name);
            typedef calc_length<base_global> functor_t;
            functor_t::params p1;
            p1.m_convertor = &m_convertor;
            p1.m_rect = &m_rect;
            functor_t fun(p1);

            f.ForEachPointRef(fun, m_zoom);

            textLength += 50;

            if ((fun.IsExist()) && (fun.m_length > textLength))
            {
              p.m_startLength = (fun.m_length - textLength) / 2;
              p.m_endLength = p.m_startLength + textLength;
            }
          }
        }

        functor_t fun(p);

        GET_POINTS(f, ForEachPointRef, fun, assign_path)

        break;
      }
    }

    if (isExist)
      pDrawer->Draw(ptr.get(), rules.data(), count);

    return true;
  }
}
