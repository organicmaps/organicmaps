#include "text_element.hpp"
#include "screen.hpp"
#include "skin.hpp"
#include "overlay_renderer.hpp"
#include "resource_style.hpp"

#include "../base/logging.hpp"

namespace yg
{
  TextElement::TextElement(Params const & p)
    : OverlayElement(p),
      m_fontDesc(p.m_fontDesc),
      m_logText(p.m_logText),
      m_log2vis(p.m_log2vis),
      m_glyphCache(p.m_glyphCache)
  {
    if (m_log2vis)
      m_visText = m_glyphCache->log2vis(m_logText);
    else
      m_visText = m_logText;
  }

  strings::UniString const & TextElement::logText() const
  {
    return m_logText;
  }

  strings::UniString const & TextElement::visText() const
  {
    return m_visText;
  }

  FontDesc const & TextElement::fontDesc() const
  {
    return m_fontDesc;
  }

  void TextElement::drawTextImpl(GlyphLayout const & layout, gl::OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m, FontDesc const & fontDesc, double depth) const
  {
    if ((layout.firstVisible() != 0) || (layout.lastVisible() != visText().size()))
      return;

    m2::PointD pv = pivot() * m;

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      Skin * skin = screen->skin().get();
      GlyphLayoutElem const & elem = layout.entries()[i];
      GlyphKey glyphKey(elem.m_sym, fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color);
      uint32_t const glyphID = skin->mapGlyph(glyphKey, screen->glyphCache());
      CharStyle const * charStyle = static_cast<CharStyle const *>(skin->fromID(glyphID));

      screen->drawGlyph(elem.m_pt + pv, m2::PointD(0.0, 0.0), elem.m_angle, 0, charStyle, depth);
    }
  }

  StraightTextElement::StraightTextElement(Params const & p)
    : TextElement(p),
      m_glyphLayout(p.m_glyphCache,
        p.m_fontDesc,
        p.m_pivot,
        visText(),
        p.m_position)
  {
    setPivot(m_glyphLayout.pivot());
  }

  StraightTextElement::StraightTextElement(StraightTextElement const & src, math::Matrix<double, 3, 3> const & m)
    : TextElement(src),
      m_glyphLayout(src.m_glyphLayout)
  {
    m_glyphLayout.setPivot(m_glyphLayout.pivot() * m);
    setPivot(m_glyphLayout.pivot());
  }

  m2::AARectD const StraightTextElement::boundRect() const
  {
    return m_glyphLayout.limitRect();
  }

  void StraightTextElement::draw(gl::OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m) const
  {
    if (screen->isDebugging())
    {
      yg::Color c(255, 255, 255, 32);

      if (isFrozen())
        c = yg::Color(0, 0, 255, 64);
      if (isNeedRedraw())
        c = yg::Color(255, 0, 0, 64);

      screen->drawRectangle(boundRect(), c, yg::maxDepth - 3);
    }
    else
      if (!isNeedRedraw())
        return;

    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m, m_fontDesc, yg::maxDepth);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, m, desc, yg::maxDepth);
  }

  void StraightTextElement::offset(m2::PointD const & offs)
  {
    TextElement::offset(offs);
    m_glyphLayout.setPivot(pivot());
  }

  PathTextElement::PathTextElement(Params const & p)
    : TextElement(p),
      m_glyphLayout(p.m_glyphCache,
        p.m_fontDesc,
        p.m_pts,
        p.m_ptsCount,
        visText(),
        p.m_fullLength,
        p.m_pathOffset,
        p.m_position)
  {
    setPivot(m_glyphLayout.pivot());
  }

  PathTextElement::PathTextElement(PathTextElement const & src, math::Matrix<double, 3, 3> const & m)
    : TextElement(src),
      m_glyphLayout(src.m_glyphLayout, m)
  {
    setPivot(m_glyphLayout.pivot());
  }

  m2::AARectD const PathTextElement::boundRect() const
  {
    return m2::Inflate(m_glyphLayout.limitRect(), m2::PointD(40, 2)); //< to create more sparse street names structure
  }

  void PathTextElement::draw(gl::OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m) const
  {
    if (screen->isDebugging())
    {
      yg::Color c(255, 255, 255, 32);

      if (isFrozen())
        c = yg::Color(0, 0, 255, 64);
      if (isNeedRedraw())
        c = yg::Color(255, 0, 0, 64);

      screen->drawRectangle(boundRect(), c, yg::maxDepth - 3);
    }

    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m, m_fontDesc, yg::maxDepth);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, m, desc, yg::maxDepth);
  }

  void PathTextElement::offset(m2::PointD const & offs)
  {
    TextElement::offset(offs);
    m_glyphLayout.setPivot(pivot());
  }
}
