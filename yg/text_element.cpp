#include "text_element.hpp"
#include "screen.hpp"
#include "skin.hpp"
#include "skin_page.hpp"
#include "resource_manager.hpp"
#include "overlay_renderer.hpp"
#include "resource_style.hpp"

#include "../base/logging.hpp"

namespace yg
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
                                 gl::OverlayRenderer * screen,
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

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      Skin * skin = screen->skin().get();
      GlyphLayoutElem const & elem = layout.entries()[i];
      GlyphKey glyphKey(elem.m_sym,
                        fontDesc.m_size,
                        fontDesc.m_isMasked,
                        fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color);
      uint32_t const glyphID = skin->mapGlyph(glyphKey, screen->glyphCache());
      GlyphStyle const * glyphStyle = static_cast<GlyphStyle const *>(skin->fromID(glyphID));

      m2::PointD glyphPt;
      ang::AngleD glyphAngle;

      if (doTransformPivotOnly)
      {
#ifdef USING_GLSL
        m2::PointD offsPt = offs + elem.m_pt;
        m2::PointD fullPt = pv + offs + elem.m_pt;

        offsPt.x -= fullPt.x - floor(fullPt.x);
        offsPt.y -= fullPt.y - floor(fullPt.y);

        screen->drawStraightGlyph(pv, offsPt, glyphStyle, depth);
#else
        glyphPt = pv + offs + elem.m_pt;
        glyphAngle = elem.m_angle;

        screen->drawGlyph(glyphPt, m2::PointD(0.0, 0.0), glyphAngle, 0, glyphStyle, depth);
#endif
      }
      else
      {
        glyphPt = (pv + offs + elem.m_pt) * m;
        glyphAngle = ang::AngleD(elem.m_angle.val() + deltaA);

        screen->drawGlyph(glyphPt, m2::PointD(0.0, 0.0), glyphAngle, 0, glyphStyle, depth);
      }
    }
  }
}
