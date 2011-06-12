#include "text_element.hpp"
#include "screen.hpp"
#include "skin.hpp"
#include "text_renderer.hpp"
#include "resource_style.hpp"

#include "../3party/fribidi/lib/fribidi-deprecated.h"

#include "../base/logging.hpp"

namespace yg
{
  OverlayElement::OverlayElement(Params const & p)
    : m_pivot(p.m_pivot), m_position(p.m_position), m_depth(p.m_depth)
  {}

  void OverlayElement::offset(m2::PointD const & offs)
  {
    m_pivot += offs;
  }

  m2::PointD const & OverlayElement::pivot() const
  {
    return m_pivot;
  }

  void OverlayElement::setPivot(m2::PointD const & pivot)
  {
    m_pivot = pivot;
  }

  yg::EPosition OverlayElement::position() const
  {
    return m_position;
  }

  void OverlayElement::setPosition(yg::EPosition pos)
  {
    m_position = pos;
  }

  double OverlayElement::depth() const
  {
    return m_depth;
  }

  void OverlayElement::setDepth(double depth)
  {
    m_depth = depth;
  }

  strings::UniString TextElement::log2vis(strings::UniString const & str)
  {
    size_t const count = str.size();
    strings::UniString res(count);
    FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
    fribidi_log2vis(&str[0], count, &dir, &res[0], 0, 0, 0);
    return res;
  }

  TextElement::TextElement(Params const & p)
    : OverlayElement(p),
      m_fontDesc(p.m_fontDesc),
      m_logText(p.m_logText),
      m_log2vis(p.m_log2vis),
      m_rm(p.m_rm),
      m_skin(p.m_skin)
  {
    if (m_log2vis)
      m_visText = log2vis(m_logText);
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

  void TextElement::drawTextImpl(GlyphLayout const & layout, gl::TextRenderer * screen, FontDesc const & fontDesc, double depth) const
  {
    if (layout.lastVisible() != visText().size())
      return;

    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      shared_ptr<Skin> const & skin = screen->skin();
      GlyphLayoutElem const & elem = layout.entries()[i];
      GlyphKey glyphKey(elem.m_sym, fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color);
      uint32_t const glyphID = skin->mapGlyph(glyphKey, fontDesc.m_isStatic);
      CharStyle const * charStyle = static_cast<CharStyle const *>(skin->fromID(glyphID));

      screen->drawGlyph(elem.m_pt, m2::PointD(0.0, 0.0), elem.m_angle, 0, charStyle, depth);
    }
  }

  StraightTextElement::StraightTextElement(Params const & p)
    : TextElement(p),
      m_glyphLayout(p.m_rm,
        p.m_skin,
        p.m_fontDesc,
        p.m_pivot,
        visText(),
        p.m_position)
  {
  }

  m2::AARectD const StraightTextElement::boundRect() const
  {
    return m2::AARectD(m_glyphLayout.limitRect());
  }

  void StraightTextElement::draw(gl::TextRenderer * screen) const
  {
    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m_fontDesc, yg::maxDepth);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, desc, yg::maxDepth);
  }

  void StraightTextElement::draw(gl::Screen * screen) const
  {
    draw((gl::TextRenderer*)screen);
  }

  void StraightTextElement::offset(m2::PointD const & offs)
  {
    TextElement::offset(offs);
    m_glyphLayout.offset(offs);
  }

  PathTextElement::PathTextElement(Params const & p)
    : TextElement(p),
      m_glyphLayout(p.m_rm,
        p.m_fontDesc,
        p.m_pts,
        p.m_ptsCount,
        visText(),
        p.m_fullLength,
        p.m_pathOffset,
        p.m_position)
  {
  }

  m2::AARectD const PathTextElement::boundRect() const
  {
    return m2::Inflate(m_glyphLayout.limitRect(), m2::PointD(40, 2));
  }

  void PathTextElement::draw(gl::TextRenderer * screen) const
  {
    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m_fontDesc, depth());
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, desc, depth());
  }

  void PathTextElement::draw(gl::Screen * screen) const
  {
    draw((gl::TextRenderer*)screen);
  }

  void PathTextElement::offset(m2::PointD const & offs)
  {
    TextElement::offset(offs);
    m_glyphLayout.offset(offs);
  }
}
