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
    : TextElement(p)
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

  vector<m2::AnyRectD> const & PathTextElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      m_boundRects.reserve(m_glyphLayout.boundRects().size());

      for (unsigned i = 0; i < m_glyphLayout.boundRects().size(); ++i)
        m_boundRects.push_back(m_glyphLayout.boundRects()[i]);

      //for (unsigned i = 0; i < m_boundRects.size(); ++i)
      //  m_boundRects[i] = m2::Inflate(m_boundRects[i], m2::PointD(10, 10));
      //m_boundRects[i].m2::Inflate(m2::PointD(40, 2)); //< to create more sparse street names structure
      setIsDirtyRect(false);
    }

    return m_boundRects;
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

      screen->drawRectangle(roughBoundRect(), graphics::Color(255, 255, 0, 64), depth() + doffs++);

      for (unsigned i = 0; i < boundRects().size(); ++i)
        screen->drawRectangle(boundRects()[i], c, depth() + doffs++);
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

  void PathTextElement::setPivot(m2::PointD const & pivot)
  {
    TextElement::setPivot(pivot);
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
