#include "draw_processor.hpp"

#include "../geometry/screenbase.hpp"
#include "../geometry/rect_intersect.hpp"

#include "../base/start_mem_debug.hpp"

namespace get_pts {

using namespace di;

m2::PointD base::g2p(m2::PointD const & pt) const
{
  return m_convertor.GtoP(pt);
}

base_screen::base_screen(ScreenBase const & convertor, m2::RectD const & rect)
: base(convertor)
{
  m_convertor.GtoP(rect, m_rect);
}

void one_point::operator() (CoordPointT const & p)
{
  ASSERT ( !m_exist, ("point feature should have only one point") );

  m2::PointD pt(make_point(p));

  if (m_rect.IsPointInside(pt))
  {
    m_exist = true;
    m_point = convert_point(pt);
  }
  else m_exist = false;
}

void path_points::StartPL()
{
  EndPL();

  m_points.push_back(PathInfo());
  push_point(m_prev);

  m_newPL = false;
}

void path_points::EndPL()
{
  if (!m_points.empty() && m_points.back().size() < 2)
    m_points.pop_back();
}

void path_points::simple_filtration(m2::PointD const & pt)
{
  if (m_prevPt)
  {
    if (!m2::RectD(m_prev, pt).IsIntersect(m_rect))
      m_newPL = true;
    else
    {
      if (m_newPL)
        StartPL();

      push_point(pt);
    }
  }
  else
    m_prevPt = true;

  m_prev = pt;
}

void path_points::best_filtration(m2::PointD const & pt)
{
  if (m_prevPt)
  {
    m2::PointD prev = m_prev;
    m2::PointD curr = pt;
    if (!m2::Intersect(m_rect, prev, curr))
      m_newPL = true;
    else
    {
      if (!equal_glb_pts(prev, m_prev))
      {
        m_prev = prev;
        m_newPL = true;
      }

      if (m_newPL)
        StartPL();

      push_point(curr);

      if (!equal_glb_pts(curr, pt))
        m_newPL = true;
    }
  }
  else
    m_prevPt = true;

  m_prev = pt;
}

void path_points::operator() (m2::PointD const & p)
{
  // Choose simple (fast) or best (slow) filtration

  //simple_filtration(p);
  best_filtration(p);
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

area_path_points::area_path_points(ScreenBase const & convertor, m2::RectD const & rect)
: area_base(convertor, rect)
{
  m_points.push_back(AreaInfo());
}

void area_path_points::operator() (m2::PointD const & p)
{
  push_point(p);
}

bool area_path_points::IsExist() const
{
  return (m_points.back().size() >= 3);
}

}
