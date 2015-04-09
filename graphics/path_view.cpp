#include "graphics/path_view.hpp"

namespace graphics
{
  PathPoint::PathPoint(int i, ang::AngleD const & segAngle, m2::PointD const & pt)
    : m_i(i),
      m_segAngle(segAngle),
      m_pt(pt)
  {}

  PivotPoint::PivotPoint(ang::AngleD const & angle, PathPoint const & pp)
    : m_angle(angle), m_pp(pp)
  {}

  PathView::PathView()
    : m_pts(0),
      m_ptsCount(0),
      m_isReverse(false)
  {}

  PathView::PathView(m2::PointD const * pts,
                     size_t ptsCount)
    : m_pts(pts),
      m_ptsCount(ptsCount),
      m_isReverse(false)
  {
  }

  size_t PathView::size() const
  {
    return m_ptsCount;
  }

  m2::PointD const & PathView::get(size_t i) const
  {
    ASSERT(i < m_ptsCount, ("index out of range"));
    return m_pts[m_isReverse ? m_ptsCount - i - 1 : i];
  }

  void PathView::setIsReverse(bool flag)
  {
    m_isReverse = flag;
  }

  bool PathView::isReverse() const
  {
    return m_isReverse;
  }

  PathPoint const PathView::offsetPoint(PathPoint const & pp, double offset) const
  {
    PathPoint res = pp;

    if (res.m_i == -1)
      return res;

    if (offset == 0)
      return pp;

    bool found = false;

    for (size_t i = res.m_i; i < size() - 1; ++i)
    {
      double l = res.m_pt.Length(get(i + 1));

      if (offset <= l)
      {
        if (i != res.m_i)
          res.m_segAngle = ang::AngleD(ang::AngleTo(get(i), get(i + 1)));

        res.m_pt = res.m_pt.Move(offset, res.m_segAngle.sin(), res.m_segAngle.cos());
        res.m_i = i;
        found = true;
        break;
      }
      else
      {
        offset -= l;
        res.m_pt = get(i + 1);
      }
    }

    if (!found)
      res.m_i = -1;

    return res;
  }

  PivotPoint PathView::findPivotPoint(PathPoint const & pp, double advance) const
  {
    PathPoint startPt = pp;

    PivotPoint res;
    if (startPt.m_i == -1)
      return res;

    m2::PointD pt1 = startPt.m_pt;

    double angle = 0;

    int j = startPt.m_i;

    if (advance < 0)
    {
      res.m_angle = startPt.m_segAngle;
      res.m_pp.m_segAngle = startPt.m_segAngle;
      res.m_pp.m_i = j;
      res.m_pp.m_pt = pt1.Move(advance, startPt.m_segAngle.sin(), startPt.m_segAngle.cos());
      return res;
    }

    while (advance > 0)
    {
      if (j + 1 == size())
        return res;

      double l = get(j + 1).Length(pt1);

      double segAngle = j == startPt.m_i ? startPt.m_segAngle.val()
                                         : ang::AngleTo(get(j), get(j + 1));

      angle += segAngle;

      if (l < advance)
      {
        advance -= l;
        pt1 = get(j + 1);
        ++j;
      }
      else
      {
        ang::AngleD a(segAngle);

        res.m_pp.m_i = j;
        res.m_pp.m_pt = pt1.Move(advance, a.sin(), a.cos());
        advance = 0;

        angle /= (res.m_pp.m_i - startPt.m_i + 1);
        res.m_angle = ang::AngleD(angle);
        res.m_pp.m_segAngle = a;

        break;
      }
    }

    return res;
  }

  PathPoint const PathView::front() const
  {
    return PathPoint(0, ang::AngleD(ang::AngleTo(get(0), get(1))), get(0));
  }
}
