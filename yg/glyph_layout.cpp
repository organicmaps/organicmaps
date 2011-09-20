#include "glyph_layout.hpp"
#include "font_desc.hpp"
#include "resource_style.hpp"

#include "../base/logging.hpp"
#include "../base/math.hpp"
#include "../std/sstream.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/aa_rect2d.hpp"
#include "../base/thread.hpp"

namespace yg
{
  double GlyphLayout::getKerning(GlyphLayoutElem const & prevElem, GlyphMetrics const & prevMetrics, GlyphLayoutElem const & curElem, GlyphMetrics const & curMetrics)
  {
    double res = 0;
    /// check, whether we should offset this symbol slightly
    /// not to overlap the previous symbol

    m2::AARectD prevSymRectAA(
          prevElem.m_pt.Move(prevMetrics.m_height, -prevElem.m_angle.cos(), prevElem.m_angle.sin()), //< moving by angle = prevElem.m_angle - math::pi / 2
          prevElem.m_angle,
          m2::RectD(prevMetrics.m_xOffset,
                    prevMetrics.m_yOffset,
                    prevMetrics.m_xOffset + prevMetrics.m_width,
                    prevMetrics.m_yOffset + prevMetrics.m_height));

    m2::AARectD curSymRectAA(
          curElem.m_pt.Move(curMetrics.m_height, -curElem.m_angle.cos(), curElem.m_angle.sin()), //< moving by angle = curElem.m_angle - math::pi / 2
          curElem.m_angle,
          m2::RectD(curMetrics.m_xOffset,
                    curMetrics.m_yOffset,
                    curMetrics.m_xOffset + curMetrics.m_width,
                    curMetrics.m_yOffset + curMetrics.m_height)
          );

    if (prevElem.m_angle.val() == curElem.m_angle.val())
      return res;

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

  GlyphLayout::GlyphLayout()
  {}

  GlyphLayout::GlyphLayout(GlyphCache * glyphCache,
                           FontDesc const & fontDesc,
                           m2::PointD const & pt,
                           strings::UniString const & visText,
                           yg::EPosition pos)
    : m_firstVisible(0),
      m_lastVisible(visText.size()),
      m_pivot(pt)
  {
    m2::RectD limitRect;
    m2::PointD curPt(0, 0);

    bool isFirst = true;

    for (size_t i = 0; i < visText.size(); ++i)
    {
      GlyphKey glyphKey(visText[i], fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_color);

      GlyphMetrics const m = glyphCache->getGlyphMetrics(glyphKey);

      if (isFirst)
      {
        limitRect = m2::RectD(m.m_xOffset,
                             -m.m_yOffset,
                              m.m_xOffset,
                             -m.m_yOffset);
        isFirst = false;
      }
      else
        limitRect.Add(m2::PointD(m.m_xOffset + curPt.x, -m.m_yOffset + curPt.y));

      limitRect.Add(m2::PointD(m.m_xOffset + m.m_width,
                             -(m.m_yOffset + m.m_height)) + curPt);

      GlyphLayoutElem elem;
      elem.m_sym = visText[i];
      elem.m_angle = 0;
      elem.m_pt = curPt;
      m_entries.push_back(elem);
      m_metrics.push_back(m);

      curPt += m2::PointD(m.m_xAdvance, m.m_yAdvance);
    }

    limitRect.Inflate(2, 2);

    m2::PointD ptOffs(-limitRect.SizeX() / 2 - limitRect.minX(),
                      -limitRect.SizeY() / 2 - limitRect.minY());

    /// adjusting according to position
    if (pos & EPosLeft)
      ptOffs += m2::PointD(-limitRect.SizeX() / 2, 0);
    if (pos & EPosRight)
      ptOffs += m2::PointD(limitRect.SizeX() / 2, 0);

    if (pos & EPosAbove)
      ptOffs += m2::PointD(0, -limitRect.SizeY() / 2);

    if (pos & EPosUnder)
      ptOffs += m2::PointD(0, limitRect.SizeY() / 2);

    m_limitRect = m2::AARectD(limitRect);

    for (unsigned i = 0; i < m_entries.size(); ++i)
      m_entries[i].m_pt += ptOffs;

    m_limitRect.Offset(ptOffs);
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
      m_limitRect(m2::RectD(0, 0, 0, 0)),
      m_pivot(0, 0)
  {
    m_fullLength = (m2::PointD(src.m_fullLength, 0) * m).Length(m2::PointD(0, 0) * m);
    m_pathOffset = (m2::PointD(src.m_pathOffset, 0) * m).Length(m2::PointD(0, 0) * m);
    recalcAlongPath();
  }

  GlyphLayout::GlyphLayout(GlyphCache * glyphCache,
                           FontDesc const & fontDesc,
                           m2::PointD const * pts,
                           size_t ptsCount,
                           strings::UniString const & visText,
                           double fullLength,
                           double pathOffset,
                           yg::EPosition pos)
    : m_firstVisible(0),
      m_lastVisible(0),
      m_path(pts, ptsCount, fullLength, pathOffset),
      m_fullLength(fullLength),
      m_pathOffset(pathOffset),
      m_visText(visText),
      m_pos(pos),
      m_fontDesc(fontDesc),
      m_limitRect(m2::RectD(0, 0, 0, 0)),
      m_pivot(0, 0)
  {
    for (size_t i = 0; i < m_visText.size(); ++i)
      m_metrics.push_back(glyphCache->getGlyphMetrics(GlyphKey(visText[i], m_fontDesc.m_size, m_fontDesc.m_isMasked, yg::Color(0, 0, 0, 0))));
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

    if (m_fullLength < strLength)
      return;

    PathPoint arrPathStart(0, ang::AngleD(ang::AngleTo(m_path.get(0), m_path.get(1))), m_path.get(0));

    m_pivot = m_path.offsetPoint(arrPathStart, m_fullLength / 2.0).m_pt;

    // offset of the text from path's start
    double offset = (m_fullLength - strLength) / 2.0;

    if (m_pos & yg::EPosLeft)
    {
      offset = 0;
      m_pivot = arrPathStart.m_pt;
    }

    if (m_pos & yg::EPosRight)
    {
      offset = (m_fullLength - strLength);
      m_pivot = m_path.get(m_path.size() - 1);
    }

    // calculate base line offset
    double blOffset = 2 - m_fontDesc.m_size / 2;
    // on-path kerning should be done for baseline-centered glyphs
    //double kernOffset = blOffset;

    if (m_pos & yg::EPosUnder)
      blOffset = 2 - m_fontDesc.m_size;
    if (m_pos & yg::EPosAbove)
      blOffset = 2;

    offset -= m_pathOffset;
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

    computeMinLimitRect();

//    recalcPivot();
  }

  void GlyphLayout::recalcPivot()
  {

  }

  void GlyphLayout::computeMinLimitRect()
  {
    map<double, m2::AARectD> rects;

    for (unsigned i = m_firstVisible; i < m_lastVisible; ++i)
    {
      if (m_metrics[i].m_width != 0)
      {
        map<double, m2::AARectD>::iterator it = rects.find(m_entries[i].m_angle.val());

        if (it == rects.end())
        {
          m2::AARectD symRectAA(
                m_entries[i].m_pt.Move(m_metrics[i].m_height, -m_entries[i].m_angle.cos(), m_entries[i].m_angle.sin()), //< moving by angle = m_entries[i].m_angle - math::pi / 2
                m_entries[i].m_angle,
                m2::RectD(m_metrics[i].m_xOffset,
                          m_metrics[i].m_yOffset,
                          m_metrics[i].m_xOffset + m_metrics[i].m_width,
                          m_metrics[i].m_yOffset + m_metrics[i].m_height
                          ));

          rects[m_entries[i].m_angle.val()] = symRectAA;
        }
      }
    }

    for (unsigned i = m_firstVisible; i < m_lastVisible; ++i)
    {
      if (m_metrics[i].m_width != 0)
      {
        m2::AARectD symRectAA(
              m_entries[i].m_pt.Move(m_metrics[i].m_height, -m_entries[i].m_angle.cos(), m_entries[i].m_angle.sin()), //< moving by angle = m_entries[i].m_angle - math::pi / 2
              m_entries[i].m_angle,
              m2::RectD(m_metrics[i].m_xOffset,
                        m_metrics[i].m_yOffset,
                        m_metrics[i].m_xOffset + m_metrics[i].m_width,
                        m_metrics[i].m_yOffset + m_metrics[i].m_height));

        for (map<double, m2::AARectD>::iterator it = rects.begin(); it != rects.end(); ++it)
          it->second.Add(symRectAA);
      }
    }

    double square = numeric_limits<double>::max();

    for (map<double, m2::AARectD>::iterator it = rects.begin(); it != rects.end(); ++it)
    {
      m2::RectD r = it->second.GetLocalRect();
      if (square > r.SizeX() * r.SizeY())
      {
        m_limitRect = it->second;
        square = r.SizeX() * r.SizeY();
      }
    }

    m2::PointD zero = m_limitRect.zero();
    zero = m_limitRect.ConvertFrom(zero);

    double dx = zero.x - floor(zero.x);
    double dy = zero.y - floor(zero.y);

    m_limitRect.Offset(m2::PointD(-dx, -dy));
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

  vector<GlyphMetrics> const & GlyphLayout::metrics() const
  {
    return m_metrics;
  }

  m2::AARectD const GlyphLayout::limitRect() const
  {
    return m2::Offset(m_limitRect, pivot());
  }

  m2::PointD const & GlyphLayout::pivot() const
  {
    return m_pivot;
  }

  void GlyphLayout::setPivot(m2::PointD const & pivot)
  {
    m_pivot = pivot;
  }
}
