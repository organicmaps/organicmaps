#pragma once
#include "draw_info.hpp"

#include "../indexer/cell_id.hpp" // CoordPointT

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/list.hpp"
#include "../std/limits.hpp"

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
    ScreenBase const & m_convertor;

    static m2::PointD make_point(CoordPointT const & p)
    {
      return m2::PointD(p.first, p.second);
    }
    m2::PointD g2p(m2::PointD const & pt) const;

    base(ScreenBase const & convertor) : m_convertor(convertor)
    {
    }
  };

  /// in global coordinates
  class base_global : public base
  {
  protected:
    m2::RectD const & m_rect;

    m2::PointD convert_point(m2::PointD const & pt) const
    {
      return g2p(pt);
    }

    base_global(ScreenBase const & convertor, m2::RectD const & rect)
      : base(convertor), m_rect(rect)
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

    base_screen(ScreenBase const & convertor, m2::RectD const & rect);
  };
  //@}

  class one_point : public base_global
  {
    bool m_exist;

  public:
    m2::PointD m_point;

    one_point(ScreenBase const & convertor, m2::RectD const & rect)
      : base_global(convertor, rect), m_exist(false)
    {
    }

    void operator() (CoordPointT const & p);

    bool IsExist() const { return m_exist; }
  };

  template <class TInfo, class TBase> class geometry_base : public TBase
  {
  public:
    list<TInfo> m_points;

    geometry_base(ScreenBase const & convertor, m2::RectD const & rect)
      : TBase(convertor, rect)
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

    bool m_newPL, m_prevPt;
    m2::PointD m_prev;

    double m_length;

    void StartPL(m2::PointD const & pt);
    void EndPL();

    static bool equal_glb_pts(m2::PointD const & p1, m2::PointD const & p2)
    {
      return p1.EqualDxDy(p2, 1.0E-12);
    }

    void simple_filtration(m2::PointD const & p);
    void best_filtration(m2::PointD const & p);

  public:
    path_points(ScreenBase const & convertor, m2::RectD const & rect)
      : base_type(convertor, rect), m_newPL(true), m_prevPt(false), m_length(0.0)
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
    area_base(ScreenBase const & convertor, m2::RectD const & rect)
      : base_type(convertor, rect)
    {
    }
  };

  /// Used for triangle draw policy.
  class area_tess_points : public area_base
  {
  public:
    area_tess_points(ScreenBase const & convertor, m2::RectD const & rect)
      : area_base(convertor, rect)
    {
    }

    void StartPrimitive(size_t ptsCount);
    void operator() (m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
    void EndPrimitive();

    bool IsExist() const;
  };

  /// Used in for poly-region draw policy.
  class area_path_points : public area_base
  {
  public:
    area_path_points(ScreenBase const & convertor, m2::RectD const & rect);

    void operator() (m2::PointD const & p);

    bool IsExist() const;
  };
  //@}

  /// Adapter for points filtering, before they will go for processing
  template <class TBase> class filter_screenpts_adapter : public TBase
  {
    m2::PointD m_prev;

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
    filter_screenpts_adapter(ScreenBase const & convertor, m2::RectD const & rect)
      : TBase(convertor, rect),
      m_prev(numeric_limits<CoordT>::min(), numeric_limits<CoordT>::min())
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

      m2::RectD r(arr[0], arr[1]);
      r.Add(arr[2]);

      if (!empty_scr_rect(r) && r.IsIntersect(this->m_rect))
        TBase::operator()(arr[0], arr[1], arr[2]);
    }
  };
}
