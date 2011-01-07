#include "../base/SRC_FIRST.hpp"

#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/drawing_rules.hpp"

#include "../base/math.hpp"

#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"


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
    }
  }

  DrawProcessor::DrawProcessor( m2::RectD const & r,
                                ScreenBase const & convertor,
                                shared_ptr<PaintEvent> paintEvent,
                                int scaleLevel)
    : m_rect(r),
      m_convertor(convertor),
      m_paintEvent(paintEvent),
      m_zoom(scaleLevel)
#ifdef PROFILER_DRAWING
      , m_drawCount(0)
#endif
  {
    m_keys.reserve(16);

    // calc current scale (pixels in 1 global unit)
    GetDrawer()->SetScale(m_zoom);
  }

  namespace
  {
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

  void DrawProcessor::PreProcessKeys()
  {
    sort(m_keys.begin(), m_keys.end(), less_key());
    m_keys.erase(unique(m_keys.begin(), m_keys.end(), equal_key()), m_keys.end());
  }

#define GET_POINTS(functor_t, for_each_fun, assign_fun) \
  {                                                     \
    functor_t fun(m_convertor, m_rect);                 \
    f.for_each_fun(fun, m_zoom);                        \
    if (fun.IsExist())                                  \
    {                                                   \
      isExist = true;                                   \
      assign_fun(ptr.get(), fun);                       \
    }                                                   \
  }

  bool DrawProcessor::operator()(FeatureType const & f)
  {
    if (m_paintEvent->isCancelled())
      throw redraw_operation_cancelled();

    // get drawing rules
    m_keys.clear();
    string names;       // for debug use only, in release it's empty
    int type = feature::GetDrawRule(f, m_zoom, m_keys, names);

    if (m_keys.empty())
    {
      // Index can pass here invisible features.
      // During indexing, features are placed at first visible scale bucket.
      // At higher scales it can become invisible - it depends on classificator.
      return true;
    }

    shared_ptr<di::DrawInfo> ptr(new di::DrawInfo(f.GetName()));

    using namespace get_pts;

    bool isExist = false;
    switch (type)
    {
    case FeatureBase::FEATURE_TYPE_POINT:
      GET_POINTS(get_pts::one_point, ForEachPointRef, assign_point)
      break;

    case FeatureBase::FEATURE_TYPE_AREA:
      GET_POINTS(filter_screenpts_adapter<area_tess_points>, ForEachTriangleExRef, assign_area)
      {
        // if area feature has any line-drawing-rules, than draw it like line
        for (size_t i = 0; i < m_keys.size(); ++i)
          if (m_keys[i].m_type == drule::line)
            goto draw_line;
        break;
      }

    draw_line:
    case FeatureBase::FEATURE_TYPE_LINE:
      GET_POINTS(filter_screenpts_adapter<path_points>, ForEachPointRef, assign_path)
      break;
    }

    // nothing to draw
    if (!isExist) return true;

    int const layer = f.GetLayer();
    DrawerYG * pDrawer = GetDrawer();

    // remove duplicating identical drawing keys
    PreProcessKeys();

    // push rules to drawing queue
    for (size_t i = 0; i < m_keys.size(); ++i)
    {
      int depth = m_keys[i].m_priority;
      if (layer != 0)
        depth = (layer * drule::layer_base_priority) + (depth % drule::layer_base_priority);

#ifdef PROFILER_DRAWING
      ++m_drawCount;
#endif

      pDrawer->Draw(ptr.get(), drule::rules().Find(m_keys[i]), depth);
    }

    return true;
  }
}
