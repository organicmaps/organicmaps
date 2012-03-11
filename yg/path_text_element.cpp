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
    setIsVisible((m_glyphLayout.firstVisible() == 0) && (m_glyphLayout.lastVisible() == visText().size()));
  }

  PathTextElement::PathTextElement(PathTextElement const & src, math::Matrix<double, 3, 3> const & m)
    : TextElement(src),
      m_glyphLayout(src.m_glyphLayout, m)
  {
    setPivot(m_glyphLayout.pivot());
    setIsVisible((m_glyphLayout.firstVisible() == 0) && (m_glyphLayout.lastVisible() == visText().size()));
  }

  vector<m2::AnyRectD> const & PathTextElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      m_boundRects.reserve(m_glyphLayout.boundRects().size());

      for (unsigned i = 0; i < m_glyphLayout.boundRects().size(); ++i)
        m_boundRects.push_back(m_glyphLayout.boundRects()[i]);

//      for (unsigned i = 0; i < m_boundRects.size(); ++i)
//        m_boundRects[i] = m2::Inflate(m_boundRects[i], m2::PointD(10, 10));
  //    m_boundRects[i].m2::Inflate(m2::PointD(40, 2)); //< to create more sparse street names structure
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

    if (desc.m_isMasked)
    {
      if (!isVisible())
        return;

      drawTextImpl(m_glyphLayout, screen, m, false, desc, yg::maxDepth - 1);
      desc.m_isMasked = false;
    }

    drawTextImpl(m_glyphLayout, screen, m, false, desc, yg::maxDepth);
  }

  void PathTextElement::offset(m2::PointD const & offs)
  {
    TextElement::offset(offs);
    m_glyphLayout.setPivot(pivot());
  }

  bool PathTextElement::find(ResourceStyleCache * stylesCache) const
  {
    yg::FontDesc desc = m_fontDesc;
    if (desc.m_isMasked)
    {
      if (!TextElement::find(m_glyphLayout, stylesCache, desc))
        return false;
      desc.m_isMasked = false;
    }

    return TextElement::find(m_glyphLayout, stylesCache, desc);
  }

  void PathTextElement::getNonPackedRects(ResourceStyleCache * stylesCache,
                                          ResourceStyleCacheContext * context,
                                          vector<m2::PointU> & v) const
  {
    yg::FontDesc desc = m_fontDesc;
    if (desc.m_isMasked)
    {
      TextElement::getNonPackedRects(m_glyphLayout, desc, stylesCache, context, v);
      desc.m_isMasked = false;
    }

    TextElement::getNonPackedRects(m_glyphLayout, desc, stylesCache, context, v);
  }

  void PathTextElement::map(ResourceStyleCache * stylesCache) const
  {
    yg::FontDesc desc = m_fontDesc;

    if (desc.m_isMasked)
    {
      TextElement::map(m_glyphLayout, stylesCache, desc);
      desc.m_isMasked = false;
    }

    TextElement::map(m_glyphLayout, stylesCache, desc);
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
