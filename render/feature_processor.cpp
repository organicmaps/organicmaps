#include "feature_processor.hpp"
#include "geometry_processors.hpp"
#include "feature_info.hpp"

#ifndef USE_DRAPE
#include "drawer.hpp"
#endif // USE_DRAPE

#include "indexer/feature_impl.hpp"
#include "indexer/feature_algo.hpp"


namespace fwork
{
#ifndef USE_DRAPE
  namespace
  {
    template <class TSrc> void assign_point(di::FeatureInfo & p, TSrc & src)
    {
      p.m_point = src.m_point;
    }

    template <class TSrc> void assign_path(di::FeatureInfo & p, TSrc & src)
    {
      p.m_pathes.swap(src.m_points);
    }

    template <class TSrc> void assign_area(di::FeatureInfo & p, TSrc & src)
    {
      p.m_areas.swap(src.m_points);

      ASSERT ( !p.m_areas.empty(), () );
      p.m_areas.back().SetCenter(src.GetCenter());
    }
  }

  FeatureProcessor::FeatureProcessor(m2::RectD const & r,
                                     ScreenBase const & convertor,
                                     shared_ptr<PaintEvent> const & e,
                                     int scaleLevel)
    : m_rect(r),
      m_convertor(convertor),
      m_paintEvent(e),
      m_zoom(scaleLevel),
      m_hasNonCoast(false),
      m_glyphCache(e->drawer()->GetGlyphCache())
  {
    GetDrawer()->SetScale(m_zoom);
  }

  bool FeatureProcessor::operator() (FeatureType const & f)
  {
    if (m_paintEvent->isCancelled())
      throw redraw_operation_cancelled();

    Drawer * pDrawer = GetDrawer();
    di::FeatureInfo fi(f, m_zoom, pDrawer->VisualScale(), m_glyphCache, &m_convertor, &m_rect);

    if (fi.m_styler.IsEmpty())
      return true;

    // Draw coastlines features only once.
    if (fi.m_styler.m_isCoastline)
    {
      if (!m_coasts.insert(fi.m_styler.m_primaryText).second)
        return true;
    }
    else
      m_hasNonCoast = true;

    using namespace gp;

    bool isExist = false;

    switch (fi.m_styler.m_geometryType)
    {
    case feature::GEOM_POINT:
    {
      typedef gp::one_point functor_t;

      functor_t::params p;
      p.m_convertor = &m_convertor;
      p.m_rect = &m_rect;

      functor_t fun(p);
      f.ForEachPointRef(fun, m_zoom);
      if (fun.IsExist())
      {
        isExist = true;
        assign_point(fi, fun);
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

      if (fi.m_styler.m_hasPointStyles)
        fun.SetCenter(feature::GetCenter(f, m_zoom));

      if (fun.IsExist())
      {
        isExist = true;
        assign_area(fi, fun);
      }

      // continue rendering as line if feature has linear styles too
      if (!fi.m_styler.m_hasLineStyles)
        break;
    }

    case feature::GEOM_LINE:
      {
        if (fi.m_styler.m_hasPathText)
        {
          typedef filter_screenpts_adapter<cut_path_intervals> functor_t;

          functor_t::params p;

          p.m_convertor = &m_convertor;
          p.m_rect = &m_rect;
          p.m_intervals = &fi.m_styler.m_intervals;

          functor_t fun(p);

          f.ForEachPointRef(fun, m_zoom);
          if (fun.IsExist())
          {
            isExist = true;
            assign_path(fi, fun);

#ifdef DEBUG
            if (fi.m_pathes.size() == 1)
            {
              double const offset = fi.m_pathes.front().GetOffset();
              for (size_t i = 0; i < fi.m_styler.m_offsets.size(); ++i)
                ASSERT_LESS_OR_EQUAL(offset, fi.m_styler.m_offsets[i], ());
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
            assign_path(fi, fun);
          }
        }
        break;
      }
    }

    if (isExist)
      pDrawer->Draw(fi);

    return true;
  }

  bool FeatureProcessor::IsEmptyDrawing() const
  {
    return (m_zoom >= feature::g_arrCountryScales[0] && !m_hasNonCoast);
  }
#endif // USE_DRAPE
}
