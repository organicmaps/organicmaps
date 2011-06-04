#include "../base/SRC_FIRST.hpp"

#include "text_element.hpp"
#include "screen.hpp"
#include "skin.hpp"
#include "text_renderer.hpp"
#include "resource_style.hpp"

#include "../3party/fribidi/lib/fribidi-deprecated.h"

#include "../base/logging.hpp"
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

  wstring const TextElement::log2vis(wstring const & str)
  {
    size_t const count = str.size();
    wstring res;
    res.resize(count);
    FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
    fribidi_log2vis(str.c_str(), count, &dir, &res[0], 0, 0, 0);
    return res;
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

  string const & TextElement::utf8Text() const
  {
    return m_utf8Text;
  }

  FontDesc const & TextElement::fontDesc() const
  {
    return m_fontDesc;
  }

  double TextElement::depth() const
  {
    return m_depth;
  }

  void TextElement::drawTextImpl(GlyphLayout const & layout, gl::TextRenderer * screen, FontDesc const & fontDesc, double depth) const
  {
    for (unsigned i = layout.firstVisible(); i < layout.lastVisible(); ++i)
    {
      shared_ptr<Skin> skin = screen->skin();
      GlyphLayoutElem elem = layout.entries()[i];
      uint32_t const glyphID = skin->mapGlyph(GlyphKey(elem.m_sym, fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color), fontDesc.m_isStatic);
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
        p.m_log2vis ? log2vis(strings::FromUtf8(p.m_utf8Text)) : strings::FromUtf8(p.m_utf8Text),
        p.m_position)
  {
  }

  m2::RectD const StraightTextElement::boundRect() const
  {
    return m_glyphLayout.limitRect();
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
        p.m_log2vis ? log2vis(strings::FromUtf8(p.m_utf8Text)) : strings::FromUtf8(p.m_utf8Text),
        p.m_fullLength,
        p.m_pathOffset,
        p.m_position)
  {
    m_pts.resize(p.m_ptsCount);
    copy(p.m_pts, p.m_pts + p.m_ptsCount, m_pts.begin());
  }

  m2::RectD const PathTextElement::boundRect() const
  {
    return m_glyphLayout.limitRect();
  }

  void PathTextElement::draw(gl::TextRenderer * screen) const
  {
/*    yg::PenInfo penInfo(yg::Color(0, 0, 0, 255), 2, 0, 0, 0);
    screen->drawPath(&m_pts[0], m_pts.size(), 0, screen->skin()->mapPenInfo(penInfo), yg::maxDepth - 2);
    if (boundRect().SizeX() > 500)
    {
      LOG(LINFO, (strings::FromUtf8(utf8Text()).c_str()));
    }
    screen->drawRectangle(boundRect(), yg::Color(rand() % 255, rand() % 255, rand() % 255, 64), yg::maxDepth - 3);
*/
    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m_fontDesc, yg::maxDepth);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, desc, yg::maxDepth);
  }

  void PathTextElement::draw(gl::Screen * screen) const
  {
    draw((gl::TextRenderer*)screen);
  }

  void PathTextElement::offset(m2::PointD const & offs)
  {
    for (unsigned i = 0; i < m_pts.size(); ++i)
      m_pts[i] += offs;
    TextElement::offset(offs);
    m_glyphLayout.offset(offs);
  }
}
