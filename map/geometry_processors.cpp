#include "geometry_processors.hpp"

#include "../std/bind.hpp"

namespace gp
{
  using namespace di;

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
    else
      m_exist = false;
  }

  get_path_intervals::params::params()
    : m_intervals(0)
  {}

  get_path_intervals::get_path_intervals(params const & p)
    : base_t(p),
      m_hasPrevPt(false),
      m_intervals(p.m_intervals),
      m_isFirst(true),
      m_length(0)
  {}

  void get_path_intervals::operator()(m2::PointD const & pt)
  {
    if (m_hasPrevPt)
    {
      m2::PointD prev = m_prev;
      m2::PointD cur = pt;

      double segLen = cur.Length(prev);

      if (m2::Intersect(base_t::m_rect, prev, cur))
      {
        double clipStart = prev.Length(m_prev) + m_length;
        double clipEnd = cur.Length(m_prev) + m_length;

        if (m_isFirst)
        {
          m_intervals->push_back(clipStart);
          m_intervals->push_back(clipEnd);
          m_isFirst = false;
        }
        else
        {
          if (fabs(m_intervals->back() - clipStart) < 1.0E-12)
            m_intervals->back() = clipEnd;
          else
          {
            m_intervals->push_back(clipStart);
            m_intervals->push_back(clipEnd);
          }
        }
      }

      m_prev = pt;
      m_length += segLen;
    }
    else
    {
      m_prev = pt;
      m_hasPrevPt = true;
    }
  }

  bool get_path_intervals::IsExist() const
  {
    return !m_isFirst;
  }

  cut_path_intervals::params::params() : m_intervals(0)
  {}

  cut_path_intervals::cut_path_intervals(params const & p)
    : base_t(p),
      m_length(0),
      m_isInside(false),
      m_hasPrevPt(false),
      m_intervals(p.m_intervals),
      m_pos(0)
  {}

  void cut_path_intervals::operator()(m2::PointD const & pt)
  {
    if (m_hasPrevPt)
    {
      double segLen = pt.Length(m_prev);

      while (true)
      {
        m2::PointD dir = (pt - m_prev) / segLen;

        if (m_pos == m_intervals->size())
          break;

        double start = (*m_intervals)[m_pos];
        double end = (*m_intervals)[m_pos + 1];

        if (start > m_length + segLen)
          break;

        if (start >= m_length)
        {
          m_points.push_back(PathInfo(start));
          push_point(m_prev + dir * (start - m_length));
          m_isInside = true;
        }

        if (end >= m_length + segLen)
        {
          push_point(pt);
          break;
        }

        if (end < m_length + segLen)
        {
          push_point(m_prev + dir * (end - m_length));
          m_isInside = false;
        }

        m_pos += 2;
      }

      m_prev = pt;
      m_length += segLen;
    }
    else
    {
      m_hasPrevPt = true;
      m_prev = pt;
    }
  }

  bool cut_path_intervals::IsExist()
  {
    // finally, assign whole length to every cutted path
    for_each(m_points.begin(), m_points.end(),
             bind(&di::PathInfo::SetFullLength, _1, m_length));

    return !m_points.empty();
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
