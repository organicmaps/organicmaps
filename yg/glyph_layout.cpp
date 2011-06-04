#include "../base/SRC_FIRST.hpp"

#include "glyph_layout.hpp"
#include "resource_manager.hpp"
#include "skin.hpp"
#include "font_desc.hpp"
#include "resource_style.hpp"
#include "text_path.hpp"

#include "../base/logging.hpp"
#include "../base/math.hpp"
#include "../std/sstream.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/aa_rect2d.hpp"

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
        res.m_pt = res.m_pt.Move(min(l, offset), ang::AngleTo(get(i), get(i + 1)));
        res.m_i = i;

        if (offset < l)
          break;
        else
          offset -= l;
      }

      return res;

    }

    PivotPoint findPivotPoint(PathPoint const & pp, GlyphMetrics const & sym, double kern)
    {
      PathPoint startPt = offsetPoint(pp, kern);

      PivotPoint res;
      res.m_pp.m_i = -1;
      m2::PointD pt1 = startPt.m_pt;

      double angle = 0;
      double advance = sym.m_xOffset + sym.m_width / 2.0;

      int j = startPt.m_i;

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

          angle /= (res.m_pp.m_i - startPt.m_i + 1);
          res.m_angle = angle;

          break;
        }
      }

      return res;
    }
  };

  double GlyphLayout::getKerning(GlyphLayoutElem const & prevElem, GlyphLayoutElem const & curElem)
  {
    double res = 0;
    /// check, whether we should offset this symbol slightly
    /// not to overlap the previous symbol

    m2::AARectD prevSymRectAA(
          prevElem.m_pt.Move(prevElem.m_metrics.m_height, prevElem.m_angle - math::pi / 2),
          prevElem.m_angle,
          m2::RectD(prevElem.m_metrics.m_xOffset,
                    prevElem.m_metrics.m_yOffset,
                    prevElem.m_metrics.m_xOffset + prevElem.m_metrics.m_width,
                    prevElem.m_metrics.m_yOffset + prevElem.m_metrics.m_height));

    m2::AARectD curSymRectAA(
          curElem.m_pt.Move(curElem.m_metrics.m_height, curElem.m_angle - math::pi / 2),
          curElem.m_angle,
          m2::RectD(curElem.m_metrics.m_xOffset,
                    curElem.m_metrics.m_yOffset,
                    curElem.m_metrics.m_xOffset + curElem.m_metrics.m_width,
                    curElem.m_metrics.m_yOffset + curElem.m_metrics.m_height)
          );

    m2::RectD prevLocalRect = prevSymRectAA.GetLocalRect();
    m2::PointD pts[4];
    prevSymRectAA.GetGlobalPoints(pts);
    curSymRectAA.ConvertTo(pts, 4);

    m2::RectD prevRectInCurSym(pts[0].x, pts[0].y, pts[0].x, pts[0].y);
    prevRectInCurSym.Add(pts[1]);
    prevRectInCurSym.Add(pts[2]);
    prevRectInCurSym.Add(pts[3]);

    m2::RectD curSymRect = curSymRectAA.GetLocalRect();

    if (curSymRect.minX() < prevRectInCurSym.maxX())
      res = prevRectInCurSym.maxX() - curSymRect.minX();

    return res;
  }

  GlyphLayout::GlyphLayout(shared_ptr<ResourceManager> const & resourceManager,
                           shared_ptr<Skin> const & skin,
                           FontDesc const & fontDesc,
                           m2::PointD const & pt,
                           wstring const & text,
                           yg::EPosition pos)
    : m_resourceManager(resourceManager),
      m_firstVisible(0),
      m_lastVisible(text.size())
  {
    m2::PointD pv = pt;

    for (size_t i = 0; i < text.size(); ++i)
    {
      GlyphKey glyphKey(text[i], fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_color);

      if (fontDesc.m_isStatic)
      {
        uint32_t glyphID = skin->mapGlyph(glyphKey, fontDesc.m_isStatic);
        CharStyle const * p = static_cast<CharStyle const *>(skin->fromID(glyphID));
        if (p != 0)
        {
          if (i == 0)
            m_limitRect = m2::RectD(p->m_xOffset + pv.x,
                                   -p->m_yOffset + pv.y,
                                    p->m_xOffset + pv.x,
                                   -p->m_yOffset + pv.y);
          else
            m_limitRect.Add(m2::PointD(p->m_xOffset, -p->m_yOffset) + pv);

          m_limitRect.Add(m2::PointD(p->m_xOffset + p->m_texRect.SizeX() - 4,
                                   -(p->m_yOffset + (int)p->m_texRect.SizeY() - 4)) + pv);

        }

        GlyphLayoutElem elem;

        elem.m_sym = text[i];
        elem.m_angle = 0;
        elem.m_pt = pv;
        elem.m_metrics.m_height = p->m_texRect.SizeY() - 4;
        elem.m_metrics.m_width = p->m_texRect.SizeX() - 4;
        elem.m_metrics.m_xAdvance = p->m_xAdvance;
        elem.m_metrics.m_xOffset = p->m_xOffset;
        elem.m_metrics.m_yOffset = p->m_yOffset;
        elem.m_metrics.m_yAdvance = 0;

        m_entries.push_back(elem);

        pv += m2::PointD(p->m_xAdvance, 0);
      }
      else
      {
        GlyphMetrics const m = resourceManager->getGlyphMetrics(glyphKey);
        if (i == 0)
          m_limitRect = m2::RectD(m.m_xOffset + pv.x,
                                 -m.m_yOffset + pv.y,
                                  m.m_xOffset + pv.x,
                                 -m.m_yOffset + pv.y);
        else
          m_limitRect.Add(m2::PointD(m.m_xOffset, -m.m_yOffset) + pv);

        m_limitRect.Add(m2::PointD(m.m_xOffset + m.m_width,
                                 -(m.m_yOffset + m.m_height)) + pv);

        GlyphLayoutElem elem;
        elem.m_sym = text[i];
        elem.m_angle = 0;
        elem.m_pt = pv;
        elem.m_metrics = m;
        m_entries.push_back(elem);

        pv += m2::PointD(m.m_xAdvance, m.m_yAdvance);
      }
    }

    m_limitRect.Inflate(2, 2);

    m2::PointD ptOffs(-m_limitRect.SizeX() / 2,
                      -m_limitRect.SizeY() / 2);

    /// adjusting according to position
    if (pos & EPosLeft)
      ptOffs += m2::PointD(-m_limitRect.SizeX() / 2, 0);
    if (pos & EPosRight)
      ptOffs += m2::PointD(m_limitRect.SizeX() / 2, 0);

    if (pos & EPosAbove)
      ptOffs += m2::PointD(0, m_limitRect.SizeY() / 2);

    if (pos & EPosUnder)
      ptOffs += m2::PointD(0, -m_limitRect.SizeY() / 2);

    offset(ptOffs);
  }


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
    TextPath arrPath(pts, ptsCount, fullLength, pathOffset);

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

    if (fullLength < strLength)
      return;

    // offset of the text from path's start
    double offset = (fullLength - strLength) / 2.0;

    if (pos & yg::EPosLeft)
      offset = 0;
    if (pos & yg::EPosRight)
      offset = (fullLength - strLength);

    // calculate base line offset
    double blOffset = 2 - fontDesc.m_size / 2;
    // on-path kerning should be done for baseline-centered glyphs
    //double kernOffset = blOffset;

    if (pos & yg::EPosUnder)
      blOffset = 2 - fontDesc.m_size;
    if (pos & yg::EPosAbove)
      blOffset = 2;

    offset -= pathOffset;
    if (-offset >= strLength)
      return;

    // find first visible glyph
    size_t symPos = 0;
    while (offset < 0 &&  symPos < count)
      offset += m_entries[symPos++].m_metrics.m_xAdvance;

    PathPoint glyphStartPt = arrPath.offsetPoint(PathPoint(0, arrPath.get(0)), offset);

    m_firstVisible = symPos;

    GlyphLayoutElem prevElem;
    bool hasPrevElem = false;

    for (; symPos < count; ++symPos)
    {
      /// full advance, including possible kerning.
      double fullGlyphAdvance = m_entries[symPos].m_metrics.m_xAdvance;

      GlyphMetrics const & metrics = m_entries[symPos].m_metrics;

      if (glyphStartPt.m_i == -1)
        return;

      if (metrics.m_width != 0)
      {
        double fullKern = 0;
        double kern = 0;

        int i = 0;
        for (; i < 100; ++i)
        {
          PivotPoint pivotPt = arrPath.findPivotPoint(glyphStartPt, metrics, fullKern);

          if (pivotPt.m_pp.m_i == -1)
            return;

          m_entries[symPos].m_angle = pivotPt.m_angle;
          double centerOffset = metrics.m_xOffset + metrics.m_width / 2.0;
          m_entries[symPos].m_pt = pivotPt.m_pp.m_pt.Move(-centerOffset, m_entries[symPos].m_angle);
          m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(blOffset, m_entries[symPos].m_angle - math::pi / 2);
//          m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(kernOffset, m_entries[symPos].m_angle - math::pi / 2);

          // < check whether we should "kern"
          if (hasPrevElem)
          {
            kern = getKerning(prevElem, m_entries[symPos]);
            if (kern < 0.5)
              kern = 0;

            fullGlyphAdvance += kern;
            fullKern += kern;
          }

          if (kern == 0)
            break;
        }
        if (i == 100)
        {
          LOG(LINFO, ("100 iteration on computing kerning exceeded. possibly infinite loop occured"));
        }

        /// kerning should be computed for baseline centered glyph
        prevElem = m_entries[symPos];
        hasPrevElem = true;

        // < align to baseline
//        m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(blOffset - kernOffset, m_entries[symPos].m_angle - math::pi / 2);
      }
      else
      {
        if (symPos == m_firstVisible)
        {
          m_firstVisible = symPos + 1;
        }
        else
        {
          m_entries[symPos].m_angle = 0;
          m_entries[symPos].m_pt = glyphStartPt.m_pt;
        }
      }

      glyphStartPt = arrPath.offsetPoint(glyphStartPt, fullGlyphAdvance);
      offset += fullGlyphAdvance;

      m_lastVisible = symPos + 1;
    }

    bool isFirst = true;

    for (unsigned i = m_firstVisible; i < m_lastVisible; ++i)
    {
      m2::AARectD symRectAA(
            m_entries[i].m_pt.Move(m_entries[i].m_metrics.m_height, m_entries[i].m_angle - math::pi / 2),
            m_entries[i].m_angle,
            m2::RectD(m_entries[i].m_metrics.m_xOffset,
                      m_entries[i].m_metrics.m_yOffset,
                      m_entries[i].m_metrics.m_xOffset + m_entries[i].m_metrics.m_width,
                      m_entries[i].m_metrics.m_yOffset + m_entries[i].m_metrics.m_height));

      m2::PointD pts[4];
      symRectAA.GetGlobalPoints(pts);

      if (isFirst)
      {
        m_limitRect = m2::RectD(pts[0].x, pts[0].y, pts[0].x, pts[0].y);
        isFirst = false;
      }
      else
        m_limitRect.Add(pts[0]);

      m_limitRect.Add(pts[1]);
      m_limitRect.Add(pts[2]);
      m_limitRect.Add(pts[3]);
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

  m2::RectD const GlyphLayout::limitRect() const
  {
    return m_limitRect;
  }

  void GlyphLayout::offset(m2::PointD const & offs)
  {
    for (unsigned i = 0; i < m_entries.size(); ++i)
      m_entries[i].m_pt += offs;
    m_limitRect.Offset(offs);
  }
}
