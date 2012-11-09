#include "glyph_layout.hpp"
#include "font_desc.hpp"
#include "resource_style.hpp"

#include "../base/logging.hpp"
#include "../base/math.hpp"
#include "../std/sstream.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../base/thread.hpp"

namespace graphics
{
  double GlyphLayout::getKerning(GlyphLayoutElem const & prevElem, GlyphMetrics const & prevMetrics, GlyphLayoutElem const & curElem, GlyphMetrics const & curMetrics)
  {
    double res = 0;
    /// check, whether we should offset this symbol slightly
    /// not to overlap the previous symbol

    m2::AnyRectD prevSymRectAA(
          prevElem.m_pt.Move(prevMetrics.m_height, -prevElem.m_angle.cos(), prevElem.m_angle.sin()), //< moving by angle = prevElem.m_angle - math::pi / 2
          prevElem.m_angle,
          m2::RectD(prevMetrics.m_xOffset,
                    prevMetrics.m_yOffset,
                    prevMetrics.m_xOffset + prevMetrics.m_width,
                    prevMetrics.m_yOffset + prevMetrics.m_height));

    m2::AnyRectD curSymRectAA(
          curElem.m_pt.Move(curMetrics.m_height, -curElem.m_angle.cos(), curElem.m_angle.sin()), //< moving by angle = curElem.m_angle - math::pi / 2
          curElem.m_angle,
          m2::RectD(curMetrics.m_xOffset,
                    curMetrics.m_yOffset,
                    curMetrics.m_xOffset + curMetrics.m_width,
                    curMetrics.m_yOffset + curMetrics.m_height)
          );

    if (prevElem.m_angle.val() == curElem.m_angle.val())
      return res;

    //m2::RectD prevLocalRect = prevSymRectAA.GetLocalRect();
    m2::PointD pts[4];
    prevSymRectAA.GetGlobalPoints(pts);
    curSymRectAA.ConvertTo(pts, 4);

    m2::RectD prevRectInCurSym(pts[0].x, pts[0].y, pts[0].x, pts[0].y);
    prevRectInCurSym.Add(pts[1]);
    prevRectInCurSym.Add(pts[2]);
    prevRectInCurSym.Add(pts[3]);

    m2::RectD const & curSymRect = curSymRectAA.GetLocalRect();

    if (curSymRect.minX() < prevRectInCurSym.maxX())
      res = prevRectInCurSym.maxX() - curSymRect.minX();

    return res;
  }

  GlyphLayout::GlyphLayout()
  {}

  GlyphLayout::GlyphLayout(GlyphCache * glyphCache,
                           FontDesc const & fontDesc,
                           m2::PointD const & pt,
                           strings::UniString const & visText,
                           graphics::EPosition pos)
    : m_firstVisible(0),
      m_lastVisible(visText.size()),
      m_fontDesc(fontDesc),
      m_pivot(pt),
      m_offset(0, 0)
  {
    m_entries.reserve(visText.size());
    m_metrics.reserve(visText.size());

    if (!m_fontDesc.IsValid())
      return;

    m2::RectD boundRect;
    m2::PointD curPt(0, 0);

    bool isFirst = true;

    for (size_t i = 0; i < visText.size(); ++i)
    {
      GlyphKey glyphKey(visText[i], fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_color);

      GlyphMetrics const m = glyphCache->getGlyphMetrics(glyphKey);

      if (isFirst)
      {
        boundRect = m2::RectD(m.m_xOffset,
                             -m.m_yOffset,
                              m.m_xOffset,
                             -m.m_yOffset);
        isFirst = false;
      }
      else
        boundRect.Add(m2::PointD(m.m_xOffset + curPt.x, -m.m_yOffset + curPt.y));

      boundRect.Add(m2::PointD(m.m_xOffset + m.m_width,
                             -(m.m_yOffset + m.m_height)) + curPt);

      GlyphLayoutElem elem;
      elem.m_sym = visText[i];
      elem.m_angle = 0;
      elem.m_pt = curPt;
      m_entries.push_back(elem);
      m_metrics.push_back(m);

      curPt += m2::PointD(m.m_xAdvance, m.m_yAdvance);
    }

    boundRect.Inflate(2, 2);

    m2::PointD ptOffs(-boundRect.SizeX() / 2 - boundRect.minX(),
                      -boundRect.SizeY() / 2 - boundRect.minY());

    /// adjusting according to position
    if (pos & EPosLeft)
      ptOffs += m2::PointD(-boundRect.SizeX() / 2, 0);

    if (pos & EPosRight)
      ptOffs += m2::PointD(boundRect.SizeX() / 2, 0);

    if (pos & EPosAbove)
      ptOffs += m2::PointD(0, -boundRect.SizeY() / 2);

    if (pos & EPosUnder)
      ptOffs += m2::PointD(0, boundRect.SizeY() / 2);

    for (unsigned i = 0; i < m_entries.size(); ++i)
      m_entries[i].m_pt += ptOffs;

    boundRect.Offset(ptOffs);

    m_boundRects.push_back(m2::AnyRectD(boundRect));
  }

  GlyphLayout::GlyphLayout(GlyphLayout const & src,
                           math::Matrix<double, 3, 3> const & m)
    : m_firstVisible(0),
      m_lastVisible(0),
      m_path(src.m_path, m),
      m_visText(src.m_visText),
      m_pos(src.m_pos),
      m_fontDesc(src.m_fontDesc),
      m_metrics(src.m_metrics),
      m_pivot(0, 0),
      m_offset(0, 0)
  {
    if (!m_fontDesc.IsValid())
      return;
    m_boundRects.push_back(m2::AnyRectD(m2::RectD(0, 0, 0, 0)));
    recalcAlongPath();
  }

  GlyphLayout::GlyphLayout(GlyphCache * glyphCache,
                           FontDesc const & fontDesc,
                           m2::PointD const * pts,
                           size_t ptsCount,
                           strings::UniString const & visText,
                           double fullLength,
                           double pathOffset,
                           graphics::EPosition pos)
    : m_firstVisible(0),
      m_lastVisible(0),
      m_path(pts, ptsCount, fullLength, pathOffset),
      m_visText(visText),
      m_pos(pos),
      m_fontDesc(fontDesc),
      m_pivot(0, 0),
      m_offset(0, 0)
  {
    if (!m_fontDesc.IsValid())
      return;
    m_boundRects.push_back(m2::AnyRectD(m2::RectD(0, 0, 0, 0)));
    for (size_t i = 0; i < m_visText.size(); ++i)
      m_metrics.push_back(glyphCache->getGlyphMetrics(GlyphKey(visText[i], m_fontDesc.m_size, m_fontDesc.m_isMasked, graphics::Color(0, 0, 0, 0))));
    recalcAlongPath();
  }

  void GlyphLayout::recalcAlongPath()
  {
    // get vector of glyphs and calculate string length
    double strLength = 0.0;
    size_t count = m_visText.size();

    if (count != 0)
      m_entries.resize(count);

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
      m_entries[i].m_sym = m_visText[i];
      strLength += m_metrics[i].m_xAdvance;
    }

    if (m_path.fullLength() < strLength)
      return;

    PathPoint arrPathStart(0, ang::AngleD(ang::AngleTo(m_path.get(0), m_path.get(1))), m_path.get(0));

    m_pivot = m_path.offsetPoint(arrPathStart, m_path.fullLength() / 2.0).m_pt;

    // offset of the text from path's start
    double offset = (m_path.fullLength() - strLength) / 2.0;

    if (m_pos & graphics::EPosLeft)
    {
      offset = 0;
      m_pivot = arrPathStart.m_pt;
    }

    if (m_pos & graphics::EPosRight)
    {
      offset = (m_path.fullLength() - strLength);
      m_pivot = m_path.get(m_path.size() - 1);
    }

    // calculate base line offset
    double blOffset = 2 - m_fontDesc.m_size / 2;
    // on-path kerning should be done for baseline-centered glyphs
    //double kernOffset = blOffset;

    if (m_pos & graphics::EPosUnder)
      blOffset = 2 - m_fontDesc.m_size;
    if (m_pos & graphics::EPosAbove)
      blOffset = 2;

    offset -= m_path.pathOffset();
    if (-offset >= strLength)
      return;

    // find first visible glyph
    size_t symPos = 0;
    while (offset < 0 &&  symPos < count)
      offset += m_metrics[symPos++].m_xAdvance;

    PathPoint glyphStartPt = m_path.offsetPoint(arrPathStart, offset);

    m_firstVisible = symPos;

    GlyphLayoutElem prevElem; //< previous glyph, to compute kerning from
    GlyphMetrics prevMetrics;
    bool hasPrevElem = false;

    for (; symPos < count; ++symPos)
    {
      /// full advance, including possible kerning.
      double fullGlyphAdvance = m_metrics[symPos].m_xAdvance;

      GlyphMetrics const & metrics = m_metrics[symPos];

      if (glyphStartPt.m_i == -1)
        return;

      if (metrics.m_width != 0)
      {
        double fullKern = 0;
        double kern = 0;

        int i = 0;
        for (; i < 100; ++i)
        {
          PivotPoint pivotPt = m_path.findPivotPoint(glyphStartPt, metrics, fullKern);

          if (pivotPt.m_pp.m_i == -1)
            return;

          m_entries[symPos].m_angle = pivotPt.m_angle;
          double centerOffset = metrics.m_xOffset + metrics.m_width / 2.0;
          m_entries[symPos].m_pt = pivotPt.m_pp.m_pt.Move(-centerOffset, m_entries[symPos].m_angle.sin(), m_entries[symPos].m_angle.cos());

//          m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(blOffset, m_entries[symPos].m_angle - math::pi / 2);
//        sin(a - pi / 2) == -cos(a)
//        cos(a - pi / 2) == sin(a)
          m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(blOffset, -m_entries[symPos].m_angle.cos(), m_entries[symPos].m_angle.sin());

//          m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(kernOffset, m_entries[symPos].m_angle - math::pi / 2);

          // < check whether we should "kern"
          if (hasPrevElem)
          {
            kern = getKerning(prevElem, prevMetrics, m_entries[symPos], m_metrics[symPos]);
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
        prevMetrics = m_metrics[symPos];
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
          m_entries[symPos].m_angle = ang::AngleD();
          m_entries[symPos].m_pt = glyphStartPt.m_pt;
        }
      }

      glyphStartPt = m_path.offsetPoint(glyphStartPt, fullGlyphAdvance);
      offset += fullGlyphAdvance;

      m_lastVisible = symPos + 1;
    }

    /// storing glyph coordinates relative to pivot point.
    for (unsigned i = m_firstVisible; i < m_lastVisible; ++i)
      m_entries[i].m_pt -= m_pivot;

    computeBoundRects();
  }

  void GlyphLayout::computeBoundRects()
  {
    map<double, m2::AnyRectD> rects;

    for (unsigned i = m_firstVisible; i < m_lastVisible; ++i)
    {
      if (m_metrics[i].m_width != 0)
      {
        ang::AngleD const & a = m_entries[i].m_angle;

        map<double, m2::AnyRectD>::iterator it = rects.find(a.val());

        m2::AnyRectD symRectAA(
              m_entries[i].m_pt.Move(m_metrics[i].m_height + m_metrics[i].m_yOffset, -a.cos(), a.sin()), //< moving by angle = m_entries[i].m_angle - math::pi / 2
              a,
              m2::RectD(m_metrics[i].m_xOffset,
                        0,
                        m_metrics[i].m_xOffset + m_metrics[i].m_width,
                        m_metrics[i].m_height
                        ));

        if (it == rects.end())
          rects[a.val()] = symRectAA;
        else
          rects[a.val()].Add(symRectAA);
      }
    }

    m_boundRects.clear();

    for (map<double, m2::AnyRectD>::const_iterator it = rects.begin(); it != rects.end(); ++it)
    {
      m2::AnyRectD r(it->second);
      m2::PointD zero = r.GlobalZero();

      double dx = zero.x - floor(zero.x);
      double dy = zero.y - floor(zero.y);

      r.Offset(m2::PointD(-dx, -dy));

      r.Offset(m_pivot);

      m_boundRects.push_back(r);
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

  buffer_vector<GlyphLayoutElem, 32> const & GlyphLayout::entries() const
  {
    return m_entries;
  }

  buffer_vector<GlyphMetrics, 32> const & GlyphLayout::metrics() const
  {
    return m_metrics;
  }

  buffer_vector<m2::AnyRectD, 16> const & GlyphLayout::boundRects() const
  {
    return m_boundRects;
  }

  m2::PointD const & GlyphLayout::pivot() const
  {
    return m_pivot;
  }

  void GlyphLayout::setPivot(m2::PointD const & pivot)
  {
    for (unsigned i = 0; i < m_boundRects.size(); ++i)
      m_boundRects[i].Offset(pivot - m_pivot);

    m_pivot = pivot;
  }

  m2::PointD const & GlyphLayout::offset() const
  {
    return m_offset;
  }

  void GlyphLayout::setOffset(m2::PointD const & offset)
  {
    for (unsigned i = 0; i < m_boundRects.size(); ++i)
      m_boundRects[i].Offset(offset - m_offset);

    m_offset = offset;
  }

  graphics::FontDesc const & GlyphLayout::fontDesc() const
  {
    return m_fontDesc;
  }
}
