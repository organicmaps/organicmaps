#include "../base/SRC_FIRST.hpp"
#include "path_text_element.hpp"
#include "overlay_renderer.hpp"

namespace yg
{
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
  //    return m2::Inflate(m_glyphLayout.limitRect(), m2::PointD(2, 2));
    return m2::Inflate(m_glyphLayout.limitRect(), m2::PointD(10, 2));
  //    return m2::Inflate(m_glyphLayout.limitRect(), m2::PointD(40, 2)); //< to create more sparse street names structure
  }

  vector<m2::AARectD> const & PathTextElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      m_boundRects.push_back(boundRect());
      setIsDirtyRect(false);
    }

    return m_boundRects;
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

      screen->drawRectangle(roughBoundRect(), yg::Color(255, 255, 0, 64), yg::maxDepth - 3);

      for (unsigned i = 0; i < boundRects().size(); ++i)
        screen->drawRectangle(boundRects()[i], c, yg::maxDepth - 3);
    }
    if (!isNeedRedraw())
      return;

    yg::FontDesc desc = m_fontDesc;
    if (m_fontDesc.m_isMasked)
    {
      if ((m_glyphLayout.firstVisible() != 0) || (m_glyphLayout.lastVisible() != visText().size()))
        return;

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

  void PathTextElement::cache(StylesCache * stylesCache) const
  {
    yg::FontDesc desc = m_fontDesc;

    if (m_fontDesc.m_isMasked)
    {
      cacheTextImpl(m_glyphLayout, stylesCache, m_fontDesc);
      desc.m_isMasked = false;
    }

    cacheTextImpl(m_glyphLayout, stylesCache, desc);
  }

  int PathTextElement::visualRank() const
  {
    return 2000 + m_fontDesc.m_size;
  }

  OverlayElement * PathTextElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new PathTextElement(*this, m);
  }
}
