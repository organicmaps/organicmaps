#include "graphics/text_path.hpp"
#include "graphics/glyph_cache.hpp"

#include "geometry/angles.hpp"


namespace graphics
{
  TextPath::TextPath()
    : m_pv(),
      m_fullLength(0),
      m_pathOffset(0)
  {}

  TextPath::TextPath(TextPath const & src, math::Matrix<double, 3, 3> const & m)
  {
    m_arr.resize(src.m_arr.size());
    for (unsigned i = 0; i < m_arr.size(); ++i)
      m_arr[i] = src.m_arr[i] * m;

    m_fullLength = (m2::PointD(src.m_fullLength, 0) * m).Length(m2::PointD(0, 0) * m);
    m_pathOffset = (m2::PointD(src.m_pathOffset, 0) * m).Length(m2::PointD(0, 0) * m);

    m_pv = PathView(&m_arr[0], m_arr.size());

    /// Fix: Check for reversing only when rotation is active,
    /// otherwise we have some flicker-blit issues for street names on zooming.
    /// @todo Should investigate this stuff.
    if (m(0, 1) != 0.0 && m(1, 0) != 0.0)
      checkReverse();
    else
      setIsReverse(src.isReverse());
  }

  TextPath::TextPath(m2::PointD const * arr, size_t sz, double fullLength, double pathOffset)
    : m_fullLength(fullLength),
      m_pathOffset(pathOffset)
  {
    ASSERT ( sz > 1, () );

    m_arr.resize(sz);
    copy(arr, arr + sz, m_arr.begin());

    m_pv = PathView(&m_arr[0], m_arr.size());

    checkReverse();
  }

  bool TextPath::isReverse() const
  {
    return m_pv.isReverse();
  }

  void TextPath::setIsReverse(bool flag)
  {
    m_pv.setIsReverse(flag);
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

      setIsReverse(true);
    }
    else
      setIsReverse(false);
  }

  double TextPath::fullLength() const
  {
    return m_fullLength;
  }

  double TextPath::pathOffset() const
  {
    return m_pathOffset;
  }

  size_t TextPath::size() const
  {
    return m_pv.size();
  }

  m2::PointD TextPath::get(size_t i) const
  {
    return m_pv.get(i);
  }

  m2::PointD TextPath::operator[](size_t i) const
  {
    return get(i);
  }

  PathPoint const TextPath::offsetPoint(PathPoint const & pp, double offset) const
  {
    return m_pv.offsetPoint(pp, offset);
  }

  void TextPath::copyWithOffset(double offset, vector<m2::PointD> & path) const
  {
    PathPoint pt = m_pv.offsetPoint(front(), m_pathOffset + offset);
    path.push_back(pt.m_pt);
    path.insert(path.end(), m_arr.begin() + pt.m_i, m_arr.end());
  }

  PivotPoint TextPath::findPivotPoint(PathPoint const & pp, GlyphMetrics const & sym) const
  {
    const PivotPoint ptStart = m_pv.findPivotPoint(pp, sym.m_xOffset - sym.m_width);
    const PivotPoint ptEnd = m_pv.findPivotPoint(pp, sym.m_xOffset + sym.m_width * 2);

    // both start and end are on the same segment, no need to calculate
    if (ptStart.m_pp.m_i == ptEnd.m_pp.m_i)
      return PivotPoint(ptStart.m_angle,
                        PathPoint(ptStart.m_pp.m_i, ptStart.m_angle, (ptStart.m_pp.m_pt + ptEnd.m_pp.m_pt) / 2.0));

    // points are on different segments, average the angle and middle point
    const PivotPoint ptMid = m_pv.findPivotPoint(pp, sym.m_xOffset + sym.m_width / 2.0);
    if ((ptStart.m_pp.m_i != -1) && (ptMid.m_pp.m_i != -1) && (ptEnd.m_pp.m_i != -1))
    {
      const ang::AngleD avgAngle(ang::GetMiddleAngle(ptStart.m_angle.val(), ptEnd.m_angle.val()));

      return PivotPoint(avgAngle,
                        PathPoint(ptMid.m_pp.m_i, ptMid.m_angle,
                                  (ptStart.m_pp.m_pt +
                                   ptMid.m_pp.m_pt + ptMid.m_pp.m_pt + // twice to compensate for long distance
                                   ptEnd.m_pp.m_pt) / 4.0));
    }
    else
    {
      // if some of the pivot points are outside of the path, just take the middle value
      return ptMid;
    }
  }

  PathPoint const TextPath::front() const
  {
    return m_pv.front();
  }
}

