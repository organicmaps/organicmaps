#include "text_element.hpp"
#include "screen.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"
#include "overlay_renderer.hpp"
#include "glyph.hpp"
#include "resource.hpp"

#include "../base/logging.hpp"

namespace graphics
{
  TextElement::TextElement(Params const & p)
    : OverlayElement(p),
      m_fontDesc(p.m_fontDesc),
      m_auxFontDesc(p.m_auxFontDesc),
      m_logText(p.m_logText),
      m_auxLogText(p.m_auxLogText),
      m_log2vis(p.m_log2vis)
  {
    if (m_log2vis)
    {
      m_visText = p.m_glyphCache->log2vis(m_logText);
      if (!m_auxLogText.empty())
        m_auxVisText = p.m_glyphCache->log2vis(m_auxLogText);
    }
    else
    {
      m_visText = m_logText;
      m_auxVisText = m_auxLogText;
    }
  }

  strings::UniString const & TextElement::logText() const
  {
    return m_logText;
  }

  strings::UniString const & TextElement::auxLogText() const
  {
    return m_auxLogText;
  }

  strings::UniString const & TextElement::visText() const
  {
    return m_visText;
  }

  strings::UniString const & TextElement::auxVisText() const
  {
    return m_auxVisText;
  }

  FontDesc const & TextElement::fontDesc() const
  {
    return m_fontDesc;
  }

  FontDesc const & TextElement::auxFontDesc() const
  {
    return m_auxFontDesc;
  }

  bool TextElement::isBidi() const
  {
    return m_logText != m_visText;
  }

  bool TextElement::isAuxBidi() const
  {
    return m_auxLogText != m_auxVisText;
  }

  void TextElement::drawTextImpl(GlyphLayout const & layout,
                                 OverlayRenderer * screen,
                                 math::Matrix<double, 3, 3> const & m,
                                 bool doTransformPivotOnly,
                                 bool doAlignPivot,
                                 FontDesc const & fontDesc,
                                 double depth) const
  {
    if (!fontDesc.IsValid())
      return;

    m2::PointD pv = layout.pivot();
    m2::PointD offs = layout.offset();
    double deltaA = 0;

    if (doTransformPivotOnly)
      pv *= m;
    else
    {
      double k = (sqrt((m(0, 0) * m(0, 0) + m(0, 1) * m(0, 1) + m(1, 0) * m(1, 0) + m(1, 1) * m(1, 1)) / 2));

      if ((k > 1.1) || (k < 1 / 1.1))
        return;

      deltaA = (ang::AngleD(0) * m).val();
    }

    size_t cnt = layout.entries().size();

    buffer_vector<Glyph::Info, 32> glyphInfos(cnt);
    buffer_vector<Resource::Info const *, 32> resInfos(cnt);
    buffer_vector<uint32_t, 32> glyphIDs(cnt);

    unsigned firstVis = layout.firstVisible();
    unsigned lastVis = layout.lastVisible();

    /// collecting all glyph infos in one array and packing them as a whole.
    for (unsigned i = firstVis; i < lastVis; ++i)
    {
      GlyphKey glyphKey(layout.entries()[i].m_sym,
                        fontDesc.m_size,
                        fontDesc.m_isMasked,
                        fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color);

      glyphInfos[i] = Glyph::Info(glyphKey, screen->glyphCache());
      resInfos[i] = &glyphInfos[i];
    }

    if ((firstVis != lastVis)
    && !screen->mapInfo(&resInfos[firstVis],
                            &glyphIDs[firstVis],
                            lastVis - firstVis))
    {
      LOG(LDEBUG, ("cannot render string", lastVis - firstVis, "characters long"));
      return;
    }

    for (unsigned i = firstVis; i < lastVis; ++i)
    {
      GlyphLayoutElem const & elem = layout.entries()[i];
      Glyph const * glyph = static_cast<Glyph const *>(screen->fromID(glyphIDs[i]));

      m2::PointD glyphPt;
      ang::AngleD glyphAngle;

      if (doTransformPivotOnly)
      {
        m2::PointD offsPt = offs + elem.m_pt;
        screen->drawStraightGlyph(pv, offsPt, glyph, depth);
      }
      else
      {
        glyphPt = (pv + offs + elem.m_pt) * m;
        glyphAngle = ang::AngleD(elem.m_angle.val() + deltaA);

        screen->drawGlyph(glyphPt, m2::PointD(0.0, 0.0), glyphAngle, 0, glyph, depth);
      }
    }
  }
}
