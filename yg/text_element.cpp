#include "../base/SRC_FIRST.hpp"

#include "text_element.hpp"
#include "screen.hpp"
#include "skin.hpp"
#include "resource_style.hpp"

#include "../base/string_utils.hpp"

namespace yg
{
  OverlayElement::OverlayElement(Params const & p)
    : m_pivot(p.m_pivot), m_position(p.m_position)
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

  TextElement::TextElement(Params const & p)
    : OverlayElement(p),
      m_fontDesc(p.m_fontDesc),
      m_utf8Text(p.m_utf8Text),
      m_depth(p.m_depth),
      m_log2vis(p.m_log2vis),
      m_rm(p.m_rm),
      m_skin(p.m_skin)
  {
  }

  void TextElement::drawTextImpl(GlyphLayout const & layout, gl::Screen * screen, FontDesc const & fontDesc)
  {
    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      shared_ptr<Skin> skin = screen->skin();
      GlyphLayoutElem elem = layout.entries()[i];
      uint32_t const glyphID = skin->mapGlyph(GlyphKey(elem.m_sym, fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color), fontDesc.m_isStatic);
      CharStyle const * charStyle = static_cast<CharStyle const *>(skin->fromID(glyphID));

      screen->drawGlyph(elem.m_pt, m2::PointD(0.0, 0.0), elem.m_angle, 0, charStyle, m_depth);
    }
  }

  StraightTextElement::StraightTextElement(Params const & p)
    : TextElement(p),
      m_glyphLayout(p.m_rm,
        p.m_skin,
        p.m_fontDesc,
        p.m_pivot,
        strings::FromUtf8(p.m_utf8Text),
        p.m_position)
  {
  }

  m2::RectD const StraightTextElement::boundRect() const
  {
    return m_glyphLayout.limitRect();
  }

  void StraightTextElement::draw(gl::Screen * screen)
  {
    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m_fontDesc);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, desc);
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
        strings::FromUtf8(p.m_utf8Text),
        p.m_fullLength,
        p.m_pathOffset,
        p.m_position)
  {
  }

  m2::RectD const PathTextElement::boundRect() const
  {
    return m_glyphLayout.limitRect();
  }

  void PathTextElement::draw(gl::Screen * screen)
  {
    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m_fontDesc);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, desc);
  }

  void PathTextElement::offset(m2::PointD const & offs)
  {
    TextElement::offset(offs);
    m_glyphLayout.offset(offs);
  }
}
