#include "path_text_element.hpp"
#include "overlay_renderer.hpp"


namespace graphics
{
  PathTextElement::Params::Params()
    : m_pts(0),
      m_ptsCount(0),
      m_fullLength(0),
      m_pathOffset(0),
      m_textOffset(0)
  {}

  PathTextElement::PathTextElement(Params const & p)
    : BaseT(p)
  {
    strings::UniString visText, auxVisText;
    (void) p.GetVisibleTexts(visText, auxVisText);

    m_glyphLayout = GlyphLayoutPath(p.m_glyphCache, p.m_fontDesc,
                                    p.m_pts, p.m_ptsCount,
                                    visText, p.m_fullLength,
                                    p.m_pathOffset, p.m_textOffset);

    setPivot(m_glyphLayout.pivot());
    setIsValid(m_glyphLayout.IsFullVisible());
  }

  m2::RectD PathTextElement::GetBoundRect() const
  {
    if (isDirtyLayout())
    {
      m_boundRect = BaseT::GetBoundRect();
      setIsDirtyLayout(false);
    }
    return m_boundRect;
  }

  void PathTextElement::GetMiniBoundRects(RectsT & rects) const
  {
    size_t const count = m_glyphLayout.boundRects().size();
    rects.reserve(count);
    copy(m_glyphLayout.boundRects().begin(),
         m_glyphLayout.boundRects().end(),
         back_inserter(rects));
  }

  void PathTextElement::draw(OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m) const
  {
    int doffs = 0;
    if (screen->isDebugging())
    {
      graphics::Color c(255, 255, 255, 32);

      if (isFrozen())
        c = graphics::Color(0, 0, 255, 64);
      if (isNeedRedraw())
        c = graphics::Color(255, 0, 0, 64);

      screen->drawRectangle(GetBoundRect(), graphics::Color(255, 255, 0, 64), depth() + doffs++);

      DrawRectsDebug(screen, c, depth() + doffs++);
    }

    if (!isNeedRedraw() || !isVisible() || !isValid())
      return;

    graphics::FontDesc desc = m_fontDesc;

    if (desc.m_isMasked)
    {
      drawTextImpl(m_glyphLayout, screen, m, false, false, desc, depth() + doffs++);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, m, false, false, desc, depth() + doffs++);
  }

  void PathTextElement::setPivot(m2::PointD const & pivot, bool dirtyFlag)
  {
    TextElement::setPivot(pivot, dirtyFlag);

    m_glyphLayout.setPivot(pivot);
  }

  void PathTextElement::setTransformation(const math::Matrix<double, 3, 3> & m)
  {
    m_glyphLayout = GlyphLayoutPath(m_glyphLayout, getResetMatrix() * m);
    TextElement::setPivot(m_glyphLayout.pivot());
    setIsValid(m_glyphLayout.IsFullVisible());

    TextElement::setTransformation(m);
  }
}
