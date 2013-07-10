#include "glyph_layout.hpp"
#include "font_desc.hpp"
#include "resource.hpp"

#include "../base/logging.hpp"
#include "../base/math.hpp"
#include "../std/sstream.hpp"

#include "../geometry/angles.hpp"
#include "../geometry/any_rect2d.hpp"
#include "../base/thread.hpp"

namespace graphics
{
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
      m_offset(0, 0),
      m_textLength(0),
      m_textOffset(0)
  {
    initStraigthText(glyphCache, fontDesc, pt, visText, pos, numeric_limits<unsigned>::max());
  }

  GlyphLayout::GlyphLayout(GlyphCache * glyphCache,
                          FontDesc const & fontDesc,
                          m2::PointD const & pt,
                          strings::UniString const & visText,
                          graphics::EPosition pos,
                          unsigned maxWidth)
    : m_firstVisible(0),
      m_lastVisible(visText.size()),
      m_fontDesc(fontDesc),
      m_pivot(pt),
      m_offset(0, 0),
      m_textLength(0),
      m_textOffset(0)
  {
    initStraigthText(glyphCache, fontDesc, pt, visText, pos, maxWidth);
  }

  GlyphLayout::GlyphLayout(GlyphLayout const & src,
                           math::Matrix<double, 3, 3> const & m)
    : m_firstVisible(0),
      m_lastVisible(0),
      m_path(src.m_path, m),
      m_visText(src.m_visText),
      m_fontDesc(src.m_fontDesc),
      m_metrics(src.m_metrics),
      m_pivot(0, 0),
      m_offset(0, 0),
      m_textLength(src.m_textLength)
  {
    if (!m_fontDesc.IsValid())
      return;

    m_boundRects.push_back(m2::AnyRectD(m2::RectD(0, 0, 0, 0)));

    m_textOffset = (m2::PointD(0, src.m_textOffset) * m).Length(m2::PointD(0, 0) * m);

    // if isReverse flag is changed, recalculate m_textOffset
    if (src.m_path.isReverse() != m_path.isReverse())
      m_textOffset = m_path.fullLength() - m_textOffset - m_textLength;

    recalcAlongPath();
  }

  GlyphLayout::GlyphLayout(GlyphCache * glyphCache,
                           FontDesc const & fontDesc,
                           m2::PointD const * pts,
                           size_t ptsCount,
                           strings::UniString const & visText,
                           double fullLength,
                           double pathOffset,
                           double textOffset)
    : m_firstVisible(0),
      m_lastVisible(0),
      m_path(pts, ptsCount, fullLength, pathOffset),
      m_visText(visText),
      m_fontDesc(fontDesc),
      m_pivot(0, 0),
      m_offset(0, 0),
      m_textLength(0),
      m_textOffset(textOffset)
  {
    if (!m_fontDesc.IsValid())
      return;

    m_boundRects.push_back(m2::AnyRectD(m2::RectD(0, 0, 0, 0)));

    size_t const cnt = m_visText.size();
    m_metrics.resize(cnt);

    for (size_t i = 0; i < cnt; ++i)
    {
      GlyphKey key(visText[i],
                   m_fontDesc.m_size,
                   false, // calculating glyph positions using the unmasked glyphs.
                   graphics::Color(0, 0, 0, 0));
      m_metrics[i] = glyphCache->getGlyphMetrics(key);
      m_textLength += m_metrics[i].m_xAdvance;
    }

    // if was reversed - recalculate m_textOffset
    if (m_path.isReverse())
      m_textOffset = m_path.fullLength() - m_textOffset - m_textLength;

    recalcAlongPath();
  }

  void GlyphLayout::addGlyph(GlyphCache * glyphCache,
                             GlyphKey const & key,
                             bool isFirst,
                             strings::UniChar symbol,
                             m2::RectD & boundRect,
                             m2::PointD & curPt)
  {
    GlyphMetrics const m = glyphCache->getGlyphMetrics(key);

    m_textLength += m.m_xAdvance;

    if (isFirst)
    {
      boundRect = m2::RectD(m.m_xOffset,
                           -m.m_yOffset,
                            m.m_xOffset,
                           -m.m_yOffset);

    }
    else
      boundRect.Add(m2::PointD(m.m_xOffset + curPt.x, -m.m_yOffset + curPt.y));

    boundRect.Add(m2::PointD(m.m_xOffset + m.m_width,
                           -(m.m_yOffset + m.m_height)) + curPt);

    GlyphLayoutElem elem;
    elem.m_sym = symbol;
    elem.m_angle = 0;
    elem.m_pt = curPt;
    m_entries.push_back(elem);
    m_metrics.push_back(m);

    curPt += m2::PointD(m.m_xAdvance, m.m_yAdvance);
  }

  void GlyphLayout::initStraigthText(GlyphCache * glyphCache,
                                     FontDesc const & fontDesc,
                                     m2::PointD const & pt,
                                     strings::UniString const & visText,
                                     graphics::EPosition pos,
                                     unsigned maxWidth)
  {
    if (!m_fontDesc.IsValid())
      return;

    size_t const cnt = visText.size();
    ASSERT_GREATER(cnt, 0, ());

    m_entries.reserve(cnt + 2);
    m_metrics.reserve(cnt + 2);

    m2::RectD boundRect;
    m2::PointD curPt(0, 0);

    bool isFirst = true;

    strings::UniChar dotSymbol = '.';
    GlyphKey dotKey(dotSymbol,
                    fontDesc.m_size,
                    false,
                    fontDesc.m_color);
    maxWidth -= glyphCache->getGlyphMetrics(dotKey).m_xAdvance;

    for (size_t i = 0; i < visText.size(); ++i)
    {
      if (m_textLength >= maxWidth)
      {
        addGlyph(glyphCache, dotKey, isFirst, dotSymbol, boundRect, curPt);
        addGlyph(glyphCache, dotKey, isFirst, dotSymbol, boundRect, curPt);
        addGlyph(glyphCache, dotKey, isFirst, dotSymbol, boundRect, curPt);
        m_lastVisible = i + 3;
        break;
      }

      GlyphKey glyphKey(visText[i],
                        fontDesc.m_size,
                        false, //< calculating glyph positions using unmasked glyphs.
                        fontDesc.m_color);

      addGlyph(glyphCache, glyphKey, isFirst, visText[i], boundRect, curPt);
      isFirst = false;
    }

    boundRect.Inflate(2, 2);

    double halfSizeX = boundRect.SizeX() / 2.0;
    double halfSizeY = boundRect.SizeY() / 2.0;

    m2::PointD ptOffs(-halfSizeX - boundRect.minX(),
                      -halfSizeY - boundRect.minY());

    // adjusting according to position
    if (pos & EPosLeft)
      ptOffs += m2::PointD(-halfSizeX, 0);

    if (pos & EPosRight)
      ptOffs += m2::PointD(halfSizeX, 0);

    if (pos & EPosAbove)
      ptOffs += m2::PointD(0, -halfSizeY);

    if (pos & EPosUnder)
      ptOffs += m2::PointD(0, halfSizeY);

    for (unsigned i = 0; i < m_entries.size(); ++i)
      m_entries[i].m_pt += ptOffs;

    boundRect.Offset(ptOffs);

    m_boundRects.push_back(m2::AnyRectD(boundRect));
  }

  void GlyphLayout::recalcAlongPath()
  {
    size_t const count = m_visText.size();
    if (count == 0 || m_path.fullLength() < m_textLength)
      return;

    m_entries.resize(count);
    for (size_t i = 0; i < count; ++i)
      m_entries[i].m_sym = m_visText[i];

    PathPoint arrPathStart = m_path.front();

    // Offset of the text from path's start.
    // In normal behaviour it should be always > 0,
    // but now we do scale tiles for the fixed layout.
    double offset = m_textOffset - m_path.pathOffset();
    if (offset < 0.0)
    {
      /// @todo Try to fix this heuristic.
      if (offset > -3.0)
        offset = 0.0;
      else
        return;
    }

    // find first visible glyph
    size_t symPos = 0;

    PathPoint glyphStartPt = m_path.offsetPoint(arrPathStart, offset);

    /// @todo Calculate better pivot (invariant point when scaling and rotating text).
    m_pivot = glyphStartPt.m_pt;

    m_firstVisible = symPos;

    GlyphLayoutElem prevElem;
    bool hasPrevElem = false;

    // calculate base line offset
    double const blOffset = 2 - m_fontDesc.m_size / 2;

    for (; symPos < count; ++symPos)
    {
      // can we find this glyph in fonts?
      if (glyphStartPt.m_i == -1)
        return;

      GlyphMetrics const & metrics = m_metrics[symPos];
      GlyphLayoutElem & entry = m_entries[symPos];

      if (metrics.m_width != 0)
      {

        PivotPoint pivotPt = m_path.findPivotPoint(glyphStartPt, metrics);

        // do we still have space on path for this glyph?
        if (pivotPt.m_pp.m_i == -1)
          return;

        entry.m_angle = pivotPt.m_angle;

        // is path too bended to be shown at all?
        if (hasPrevElem && (ang::GetShortestDistance(prevElem.m_angle.m_val, entry.m_angle.m_val) > 0.5))
          break;

        double const centerOffset = metrics.m_xOffset + metrics.m_width / 2.0;
        entry.m_pt = pivotPt.m_pp.m_pt.Move(-centerOffset,
                                            entry.m_angle.sin(),
                                            entry.m_angle.cos());

        entry.m_pt = entry.m_pt.Move(blOffset,
                                     -entry.m_angle.cos(),
                                     entry.m_angle.sin());

        // kerning should be computed for baseline centered glyph
        prevElem = entry;
        hasPrevElem = true;
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

      glyphStartPt = m_path.offsetPoint(glyphStartPt, metrics.m_xAdvance);

      m_lastVisible = symPos + 1;
    }

    // storing glyph coordinates relative to pivot point.
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

  int GlyphLayout::baseLineOffset()
  {
    int result = 0;
    for (size_t i = 0; i < m_metrics.size(); ++i)
      result = min(m_metrics[i].m_yOffset, result);

    return -result;
  }
}
