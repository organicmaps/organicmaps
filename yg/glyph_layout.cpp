#include "../base/SRC_FIRST.hpp"

#include "glyph_layout.hpp"
#include "resource_manager.hpp"
#include "font_desc.hpp"

#include "../geometry/angles.hpp"

namespace yg
{
  struct PathPoint
  {
    int m_i;
    m2::PointD m_pt;
    PathPoint(int i = -1,
              m2::PointD const & pt = m2::PointD())
            : m_i(i),
              m_pt(pt)
    {}
  };

  struct PivotPoint
  {
    double m_angle;
    PathPoint m_pp;
    PivotPoint(double angle = 0, PathPoint const & pp = PathPoint())
    {}
  };

  class pts_array
  {
    m2::PointD const * m_arr;
    size_t m_size;
    bool m_reverse;

  public:
    pts_array(m2::PointD const * arr, size_t sz, double fullLength, double & pathOffset)
      : m_arr(arr), m_size(sz), m_reverse(false)
    {
      ASSERT ( m_size > 1, () );

      /* assume, that readable text in path should be ('o' - start draw point):
       *    /   o
       *   /     \
       *  /   or  \
       * o         \
       */

      double const a = ang::AngleTo(m_arr[0], m_arr[m_size-1]);
      if (fabs(a) > math::pi / 2.0)
      {
        // if we swap direction, we need to recalculate path offset from the end
        double len = 0.0;
        for (size_t i = 1; i < m_size; ++i)
          len += m_arr[i-1].Length(m_arr[i]);

        pathOffset = fullLength - pathOffset - len;
        ASSERT ( pathOffset >= -1.0E-6, () );
        if (pathOffset < 0.0) pathOffset = 0.0;

        m_reverse = true;
      }
    }

    size_t size() const { return m_size; }

    m2::PointD get(size_t i) const
    {
      ASSERT ( i < m_size, ("Index out of range") );
      return m_arr[m_reverse ? m_size - i - 1 : i];
    }

    m2::PointD operator[](size_t i) const { return get(i); }

    PathPoint const offsetPoint(PathPoint const & pp, double offset)
    {
      PathPoint res = pp;

      if (res.m_i == -1)
        return res;

      for (size_t i = res.m_i; i < size() - 1; ++i)
      {
        double l = res.m_pt.Length(get(i + 1));
        if (offset < l)
        {
          res.m_pt = res.m_pt.Move(offset, ang::AngleTo(get(i), get(i + 1)));
          res.m_i = i;
          break;
        }
        else
        {
          offset -= l;
          res.m_pt = get(i + 1);
        }
      }

      return res;
    }

    PivotPoint findPivotPoint(PathPoint const & pp, GlyphMetrics const & sym)
    {
      PivotPoint res;
      res.m_pp.m_i = -1;
      m2::PointD pt1 = pp.m_pt;

      double angle = 0;
      double advance = sym.m_xOffset + sym.m_width / 2.0;

      int j = pp.m_i;

      while (advance > 0)
      {
        if (j + 1 == size())
          return res;

        double l = get(j + 1).Length(pt1);

        angle += ang::AngleTo(get(j), get(j + 1));

        if (l < advance)
        {
          advance -= l;
          pt1 = get(j + 1);
          ++j;
        }
        else
        {
          res.m_pp.m_i = j;
          res.m_pp.m_pt = pt1.Move(advance, ang::AngleTo(get(j), get(j + 1)));
          advance = 0;

          angle /= (res.m_pp.m_i - pp.m_i + 1);
          res.m_pp.m_pt = pt1;
          res.m_angle = angle;

          break;
        }
      }

      return res;
    }
  };

  GlyphLayout::GlyphLayout(shared_ptr<ResourceManager> const & resourceManager,
                           FontDesc const & fontDesc,
                           m2::PointD const * pts,
                           size_t ptsCount,
                           wstring const & text,
                           double fullLength,
                           double pathOffset,
                           yg::EPosition pos)
    : m_resourceManager(resourceManager),
      m_firstVisible(0),
      m_lastVisible(0)
  {
    pts_array arrPath(pts, ptsCount, fullLength, pathOffset);

    // get vector of glyphs and calculate string length
    double strLength = 0.0;
    size_t count = text.size();
    m_entries.resize(count);

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
      m_entries[i].m_sym = text[i];
      m_entries[i].m_metrics = m_resourceManager->getGlyphMetrics(GlyphKey(m_entries[i].m_sym, fontDesc.m_size, fontDesc.m_isMasked, yg::Color(0, 0, 0, 0)));
      strLength += m_entries[i].m_metrics.m_xAdvance;
    }

    // offset of the text from path's start
    double offset = (fullLength - strLength) / 2.0;
    if (offset < 0.0)
      return;
    offset -= pathOffset;
    if (-offset >= strLength)
      return;

    // find first visible glyph
    size_t symPos = 0;
    while (offset < 0 &&  symPos < count)
      offset += m_entries[symPos++].m_metrics.m_xAdvance;

    PathPoint startPt = arrPath.offsetPoint(PathPoint(0, arrPath.get(0)), offset);

    m_firstVisible = symPos;

    for (; symPos < count; ++symPos)
    {
      GlyphMetrics const & metrics = m_entries[symPos].m_metrics;

      if (startPt.m_i == -1)
        return;

      if (metrics.m_width != 0)
      {
        PivotPoint pivotPt = arrPath.findPivotPoint(startPt, metrics);

        if (pivotPt.m_pp.m_i == -1)
          return;

        m_entries[symPos].m_angle = pivotPt.m_angle;
        double centerOffset = metrics.m_xOffset + metrics.m_width / 2.0;
        //m_entries[symPos].m_pt = pivotPt.m_pp.m_pt.Move(-centerOffset, pivotPt.m_angle);
        m_entries[symPos].m_pt = startPt.m_pt;
      }

      startPt = arrPath.offsetPoint(startPt, metrics.m_xAdvance);

      m_lastVisible = symPos + 1;
    }
  }

  size_t GlyphLayout::firstVisible() const
  {
    return m_firstVisible;
  }

  size_t GlyphLayout::lastVisible() const
  {
    return m_lastVisible;
  }

  vector<GlyphLayoutElem> const & GlyphLayout::entries() const
  {
    return m_entries;
  }
}
