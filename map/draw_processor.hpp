#pragma once

#include "draw_info.hpp"
#include "events.hpp"

#include "../indexer/drawing_rule_def.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/cell_id.hpp" // CoordPointT
#include "../indexer/data_header.hpp"
#include "../indexer/feature_data.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/list.hpp"
#include "../std/limits.hpp"

#include "../yg/glyph_cache.hpp"

class ScreenBase;

namespace get_pts
{
  /// @name Base class policies (use by inheritance) for points processing.
  /// Containt next functions:\n
  /// - make_point - convert from Feature point to Screen point
  /// - convert_point - convert point to screen coordinates;\n
  /// - m_rect - clip rect;\n

  //@{
  class base
  {
  protected:
    ScreenBase const * m_convertor;

    static m2::PointD make_point(CoordPointT const & p)
    {
      return m2::PointD(p.first, p.second);
    }
    m2::PointD g2p(m2::PointD const & pt) const;

    struct params
    {
      ScreenBase const * m_convertor;
      params() : m_convertor()
      {}
    };

    base(params const & p) : m_convertor(p.m_convertor)
    {
    }
  };

  /// in global coordinates
  class base_global : public base
  {
  protected:
    m2::RectD const * m_rect;

    m2::PointD convert_point(m2::PointD const & pt) const
    {
      return g2p(pt);
    }

    struct params : base::params
    {
      m2::RectD const * m_rect;
      params() : m_rect(0){}
    };

    base_global(params const & p)
      : base(p), m_rect(p.m_rect)
    {
    }
  };

  /// in screen coordinates
  class base_screen : public base
  {
  protected:

    m2::RectD m_rect;

    m2::PointD convert_point(m2::PointD const & pt) const
    {
      return pt;
    }

    struct params : base::params
    {
      m2::RectD const * m_rect;
      params() : m_rect(0)
      {}
    };

    base_screen(params const & p);
  };

  //@}

  template <typename TBase>
  class calc_length : public TBase
  {
    bool m_exist;
    m2::PointD m_prevPt;
  public:
    double m_length;

    typedef typename TBase::params params;

    calc_length(params const & p) :
      TBase(p), m_exist(false), m_length(0)
    {}

    void operator() (CoordPointT const & p)
    {
      m2::PointD pt(this->convert_point(this->make_point(p)));
      if (m_exist)
        m_length += pt.Length(m_prevPt);

      m_exist = true;
      m_prevPt = pt;
    }

    bool IsExist() const {return m_exist;}
  };

  class one_point : public base_global
  {
    bool m_exist;

  public:
    m2::PointD m_point;

    typedef base_global::params params;

    one_point(params const & p)
      : base_global(p), m_exist(false)
    {
    }

    void operator() (CoordPointT const & p);

    bool IsExist() const { return m_exist; }
  };

  template <class TInfo, class TBase> class geometry_base : public TBase
  {
  public:
    list<TInfo> m_points;

    typedef typename TBase::params params;

    geometry_base(params const & p)
      : TBase(p)
    {
    }

    bool IsExist() const
    {
      return !m_points.empty();
    }

    void push_point(m2::PointD const & pt)
    {
      m_points.back().push_back(this->convert_point(pt));
    }
  };

  class path_points : public geometry_base<di::PathInfo, base_screen>
  {
    typedef geometry_base<di::PathInfo, base_screen> base_type;

    bool m_newPL, m_hasPrevPt;
    m2::PointD m_prev;

    double m_length;
    double m_startLength;
    double m_endLength;

    double m_fontSize;

    void StartPL(m2::PointD const & pt);
    void EndPL();

    static bool equal_glb_pts(m2::PointD const & p1, m2::PointD const & p2)
    {
      return p1.EqualDxDy(p2, 1.0E-12);
    }

    void simple_filtration(m2::PointD const & p);
    void best_filtration(m2::PointD const & p);

  public:

    struct params : base_type::params
    {
      double m_startLength;
      double m_endLength;
      params() : m_startLength(0), m_endLength(0)
      {}
    };

    path_points(params const & p)
      : base_type(p),
        m_newPL(true),
        m_hasPrevPt(false),
        m_length(0.0),
        m_startLength(p.m_startLength),
        m_endLength(p.m_endLength)
    {
    }

    void operator() (m2::PointD const & p);

    bool IsExist();
  };

  /// @name Policies for filling area.
  //@{
  class area_base : public geometry_base<di::AreaInfo, base_screen>
  {
    typedef geometry_base<di::AreaInfo, base_screen> base_type;

  public:

    typedef base_type::params params;

    area_base(params const & p)
      : base_type(p)
    {
    }
  };

  /// Used for triangle draw policy.
  class area_tess_points : public area_base
  {
  public:
    typedef area_base::params params;

    area_tess_points(params const & p)
      : area_base(p)
    {
    }

    void StartPrimitive(size_t ptsCount);
    void operator() (m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
    void EndPrimitive();

    bool IsExist() const;
  };
  //@}

  /// Adapter for points filtering, before they will go for processing
  template <class TBase> class filter_screenpts_adapter : public TBase
  {
    size_t m_count;
    m2::PointD m_prev, m_center;

    static bool equal_scr_pts(m2::PointD const & p1, m2::PointD const & p2)
    {
      return p1.EqualDxDy(p2, 0.5);
    }
    static bool empty_scr_rect(m2::RectD const & r)
    {
      double const eps = 1.0;
      return (r.SizeX() < eps && r.SizeY() < eps);
    }

  public:

    typedef typename TBase::params params;

    filter_screenpts_adapter(params const & p)
      : TBase(p), m_count(0),
      m_prev(numeric_limits<CoordT>::min(), numeric_limits<CoordT>::min()), m_center(0, 0)
    {
    }

    void operator() (CoordPointT const & p)
    {
      m2::PointD pt = this->g2p(this->make_point(p));
      if (!equal_scr_pts(m_prev, pt))
      {
        TBase::operator()(pt);
        m_prev = pt;
      }
    }

    void operator() (m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
    {
      m2::PointD arr[] = { this->g2p(p1), this->g2p(p2), this->g2p(p3) };

      m2::RectD r;
      for (int i = 0; i < 3; ++i)
      {
        r.Add(arr[i]);
        m_center += arr[i];
      }
      ++m_count;

      if (!empty_scr_rect(r) && r.IsIntersect(this->m_rect))
        TBase::operator()(arr[0], arr[1], arr[2]);
    }

    m2::PointD GetCenter() const { return m_center / (3*m_count); }
  };
}

namespace drule { class BaseRule; }
namespace di { class DrawInfo; }

class redraw_operation_cancelled {};

namespace fwork
{
  class DrawProcessor
  {
    m2::RectD m_rect;

    set<string> m_coasts;

    ScreenBase const & m_convertor;

    shared_ptr<PaintEvent> m_paintEvent;

    int m_zoom;
    bool m_hasNonCoast;
    bool m_hasAnyFeature;

    yg::GlyphCache * m_glyphCache;

#ifdef PROFILER_DRAWING
    size_t m_drawCount;
#endif

    inline DrawerYG * GetDrawer() const { return m_paintEvent->drawer(); }

    void PreProcessKeys(vector<drule::Key> & keys) const;

  public:

    DrawProcessor(m2::RectD const & r,
                  ScreenBase const & convertor,
                  shared_ptr<PaintEvent> const & paintEvent,
                  int scaleLevel);

    bool operator() (FeatureType const & f);

    bool IsEmptyDrawing() const;
  };
}
