#pragma once

#include "area_info.hpp"
#include "path_info.hpp"

#include "../indexer/cell_id.hpp" // CoordPointT

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/rect_intersect.hpp"
#include "geometry/screenbase.hpp"

#include "std/list.hpp"
#include "std/limits.hpp"

#include "base/buffer_vector.hpp"

class ScreenBase;

namespace df
{
namespace watch
{

/// @name Base class policies (use by inheritance) for points processing.
/// Containt next functions:\n
/// - make_point - convert from Feature point to Screen point
/// - convert_point - convert point to screen coordinates;\n
/// - m_rect - clip rect;\n

//@{
struct base
{
  struct params
  {
    ScreenBase const * m_convertor;
    params() : m_convertor()
    {}
  };

  base(params const & p)
    : m_convertor(p.m_convertor)
  {
  }

  ScreenBase const * m_convertor;

  m2::PointD g2p(m2::PointD const & pt) const
  {
    return m_convertor->GtoP(pt);
  }
};

/// in global coordinates
struct base_global : public base
{
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
struct base_screen : public base
{
  m2::RectD m_rect;

  struct params : base::params
  {
    m2::RectD const * m_rect;
    params() : m_rect(0)
    {}
  };

  base_screen(params const & p);

  m2::PointD convert_point(m2::PointD const & pt) const
  {
    return pt;
  }
};

//@}

template <typename TBase>
struct calc_length : public TBase
{
  bool m_exist;
  m2::PointD m_prevPt;
  double m_length;

  typedef typename TBase::params params;

  calc_length(params const & p) :
    TBase(p), m_exist(false), m_length(0)
  {}

  void operator() (m2::PointD const & p)
  {
    m2::PointD const pt(this->convert_point(p));

    if (m_exist)
      m_length += pt.Length(m_prevPt);

    m_exist = true;
    m_prevPt = pt;
  }

  bool IsExist() const
  {
    return m_exist;
  }
};

struct one_point : public base_global
{
  bool m_exist;
  m2::PointD m_point;

  typedef base_global::params params;

  one_point(params const & p)
    : base_global(p), m_exist(false)
  {
  }

  void operator() (m2::PointD const & pt);

  bool IsExist() const
  {
    return m_exist;
  }
};

template <class TInfo, class TBase>
struct geometry_base : public TBase
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
    /// @todo Filter for equal points.
    m_points.back().push_back(this->convert_point(pt));
  }
};

struct interval_params : public geometry_base<PathInfo, base_screen>
{
  typedef geometry_base<PathInfo, base_screen> base_t;

  buffer_vector<double, 16> * m_intervals;

  m2::PointD m_prev;
  double m_length;
  bool m_hasPrevPt;

  struct params : base_t::params
  {
    buffer_vector<double, 16> * m_intervals;
    params() : m_intervals(0) {}
  };

  interval_params(params const & p);
};

struct get_path_intervals : public interval_params
{
  get_path_intervals(params const & p) : interval_params(p) {}

  void operator() (m2::PointD const & pt);

  bool IsExist() const;
};

struct cut_path_intervals : public interval_params
{
  size_t m_pos;

  cut_path_intervals(params const & p) : interval_params(p), m_pos(0) {}

  void operator() (m2::PointD const & p);

  bool IsExist();
};

class path_points : public geometry_base<PathInfo, base_screen>
{
  typedef geometry_base<PathInfo, base_screen> base_type;

  bool m_newPL, m_hasPrevPt;
  m2::PointD m_prev;

  double m_length;
  double m_startLength;
  double m_endLength;

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
class area_base : public geometry_base<AreaInfo, base_screen>
{
  typedef geometry_base<AreaInfo, base_screen> base_type;

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
    : TBase(p),
    m_prev(numeric_limits<double>::min(), numeric_limits<double>::min()), m_center(0, 0)
  {
  }

  void operator() (m2::PointD const & p)
  {
    m2::PointD const pt = this->g2p(p);
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
      r.Add(arr[i]);

    if (!empty_scr_rect(r) && r.IsIntersect(this->m_rect))
      TBase::operator()(arr[0], arr[1], arr[2]);
  }

  m2::PointD GetCenter() const { return m_center; }
  void SetCenter(m2::PointD const & p) { m_center = this->g2p(p); }
};

}
}
