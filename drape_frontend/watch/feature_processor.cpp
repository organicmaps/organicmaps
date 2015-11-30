#include "drape_frontend/watch/feature_processor.hpp"
#include "drape_frontend/watch/geometry_processors.hpp"
#include "drape_frontend/watch/feature_styler.hpp"
#include "drape_frontend/watch/cpu_drawer.hpp"

#include "indexer/feature_impl.hpp"
#include "indexer/feature_algo.hpp"

namespace df
{
namespace watch
{

namespace
{
  template <class TSrc> void assign_point(FeatureData & data, TSrc & src)
  {
    data.m_point = src.m_point;
  }

  template <class TSrc> void assign_path(FeatureData & data, TSrc & src)
  {
    data.m_pathes.swap(src.m_points);
  }

  template <class TSrc> void assign_area(FeatureData & data, TSrc & src)
  {
    data.m_areas.swap(src.m_points);

    ASSERT(!data.m_areas.empty(), ());
    data.m_areas.back().SetCenter(src.GetCenter());
  }
}

FeatureProcessor::FeatureProcessor(ref_ptr<CPUDrawer> drawer,
                                   m2::RectD const & r,
                                   ScreenBase const & convertor,
                                   int zoomLevel)
  : m_drawer(drawer)
  , m_rect(r)
  , m_convertor(convertor)
  , m_zoom(zoomLevel)
  , m_hasNonCoast(false)
{
}

bool FeatureProcessor::operator()(FeatureType const & f)
{
  FeatureData data;
  data.m_id = f.GetID();
  data.m_styler = FeatureStyler(f, m_zoom, m_drawer->GetVisualScale(),
                                m_drawer->GetGlyphCache(), &m_convertor, &m_rect);

  if (data.m_styler.IsEmpty())
    return true;

  // Draw coastlines features only once.
  if (data.m_styler.m_isCoastline)
  {
    if (!m_coasts.insert(data.m_styler.m_primaryText).second)
      return true;
  }
  else
    m_hasNonCoast = true;

  bool isExist = false;

  switch (data.m_styler.m_geometryType)
  {
  case feature::GEOM_POINT:
  {
    typedef one_point functor_t;

    functor_t::params p;
    p.m_convertor = &m_convertor;
    p.m_rect = &m_rect;

    functor_t fun(p);
    f.ForEachPointRef(fun, m_zoom);
    if (fun.IsExist())
    {
      isExist = true;
      assign_point(data, fun);
    }

    break;
  }

  case feature::GEOM_AREA:
  {
    typedef filter_screenpts_adapter<area_tess_points> functor_t;

    functor_t::params p;
    p.m_convertor = &m_convertor;
    p.m_rect = &m_rect;

    functor_t fun(p);
    f.ForEachTriangleExRef(fun, m_zoom);

    if (data.m_styler.m_hasPointStyles)
      fun.SetCenter(feature::GetCenter(f, m_zoom));

    if (fun.IsExist())
    {
      isExist = true;
      assign_area(data, fun);
    }

    // continue rendering as line if feature has linear styles too
    if (!data.m_styler.m_hasLineStyles)
      break;
  }

  case feature::GEOM_LINE:
    {
      if (data.m_styler.m_hasPathText)
      {
        typedef filter_screenpts_adapter<cut_path_intervals> functor_t;

        functor_t::params p;

        p.m_convertor = &m_convertor;
        p.m_rect = &m_rect;
        p.m_intervals = &data.m_styler.m_intervals;

        functor_t fun(p);

        f.ForEachPointRef(fun, m_zoom);
        if (fun.IsExist())
        {
          isExist = true;
          assign_path(data, fun);

          #ifdef DEBUG
          if (data.m_pathes.size() == 1)
          {
            double const offset = data.m_pathes.front().GetOffset();
            for (size_t i = 0; i < data.m_styler.m_offsets.size(); ++i)
              ASSERT_LESS_OR_EQUAL(offset, data.m_styler.m_offsets[i], ());
          }
          #endif
        }
      }
      else
      {
        typedef filter_screenpts_adapter<path_points> functor_t;

        functor_t::params p;

        p.m_convertor = &m_convertor;
        p.m_rect = &m_rect;

        functor_t fun(p);

        f.ForEachPointRef(fun, m_zoom);

        if (fun.IsExist())
        {
          isExist = true;
          assign_path(data, fun);
        }
      }
      break;
    }
  }

  if (isExist)
    m_drawer->Draw(data);

  return true;
}

}
}
