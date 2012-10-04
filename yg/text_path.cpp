#include "../base/SRC_FIRST.hpp"

#include "text_path.hpp"

#include "../geometry/angles.hpp"

namespace yg
{
  PathPoint::PathPoint(int i, ang::AngleD const & segAngle, m2::PointD const & pt)
    : m_i(i),
      m_segAngle(segAngle),
      m_pt(pt)
  {}

  PivotPoint::PivotPoint(ang::AngleD const & angle, PathPoint const & pp)
    : m_angle(angle), m_pp(pp)
  {}

  TextPath::TextPath() : m_reverse(false), m_fullLength(0), m_pathOffset(0)
  {}

  TextPath::TextPath(TextPath const & src, math::Matrix<double, 3, 3> const & m)
  {
    m_arr.resize(src.m_arr.size());
    for (unsigned i = 0; i < m_arr.size(); ++i)
      m_arr[i] = src.m_arr[i] * m;

    m_fullLength = (m2::PointD(src.m_fullLength, 0) * m).Length(m2::PointD(0, 0) * m);
    m_pathOffset = (m2::PointD(src.m_pathOffset, 0) * m).Length(m2::PointD(0, 0) * m);

    /// Fix: Check for reversing only when rotation is active,
    /// otherwise we have some flicker-blit issues for street names on zooming.
    /// @todo Should investigate this stuff.
    if (m(0, 1) != 0.0 && m(1, 0) != 0.0)
    {
      m_reverse = false;
      checkReverse();
    }
    else
      m_reverse = src.m_reverse;
  }

  TextPath::TextPath(m2::PointD const * arr, size_t sz, double fullLength, double pathOffset)
    : m_reverse(false),
      m_fullLength(fullLength),
      m_pathOffset(pathOffset)
  {
    ASSERT ( sz > 1, () );

    m_arr.resize(sz);
    copy(arr, arr + sz, m_arr.begin());

    checkReverse();
  }

  void TextPath::checkReverse()
  {
    /* assume, that readable text in path should be ('o' - start draw point):
     *    /   o
     *   /     \
     *  /   or  \
     * o         \
     */

    double const a = ang::AngleTo(m_arr[0], m_arr[m_arr.size() - 1]);
    if (fabs(a) > math::pi / 2.0)
    {
      // if we swap direction, we need to recalculate path offset from the end
      double len = 0.0;
      for (size_t i = 1; i < m_arr.size(); ++i)
        len += m_arr[i-1].Length(m_arr[i]);

      m_pathOffset = m_fullLength - m_pathOffset - len;
      ASSERT ( m_pathOffset >= -1.0E-6, () );
      if (m_pathOffset < 0.0)
        m_pathOffset = 0.0;

      m_reverse = true;
    }
  }

  double TextPath::fullLength() const
  {
    return m_fullLength;
  }

  double TextPath::pathOffset() const
  {
    return m_pathOffset;
  }

  size_t TextPath::size() const { return m_arr.size(); }

  m2::PointD TextPath::get(size_t i) const
  {
    ASSERT ( i < m_arr.size(), ("Index out of range") );
    return m_arr[m_reverse ? m_arr.size() - i - 1 : i];
  }

  m2::PointD TextPath::operator[](size_t i) const { return get(i); }

  PathPoint const TextPath::offsetPoint(PathPoint const & pp, double offset)
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

  PivotPoint TextPath::findPivotPoint(PathPoint const & pp, GlyphMetrics const & sym, double kern)
  {
    PathPoint startPt = offsetPoint(pp, kern);

    PivotPoint res;
    if (startPt.m_i == -1)
      return res;

    m2::PointD pt1 = startPt.m_pt;

    double angle = 0;
    double advance = sym.m_xOffset + sym.m_width / 2.0;

    int j = startPt.m_i;

    while (advance > 0)
    {
      if (j + 1 == size())
        return res;

      double l = get(j + 1).Length(pt1);

      double segAngle = j == startPt.m_i ? startPt.m_segAngle.val() : ang::AngleTo(get(j), get(j + 1));

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
}

