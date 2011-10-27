#include "../base/logging.hpp"

#include "symbol_element.hpp"
#include "styles_cache.hpp"
#include "resource_style.hpp"
#include "overlay_renderer.hpp"
#include "skin.hpp"

namespace yg
{
  SymbolElement::SymbolElement(Params const & p)
    : base_t(p),
      m_symbolName(p.m_symbolName),
      m_symbolRect(0, 0, 0, 0)
  {
    uint32_t styleID = p.m_skin->mapSymbol(m_symbolName.c_str());
    ResourceStyle const * style = p.m_skin->fromID(styleID);

    if (style == 0)
    {
      LOG(LINFO, ("POI ", m_symbolName, " wasn't found on the current skin"));
      return;
    }

    m_symbolRect = style->m_texRect;
  }

  SymbolElement::SymbolElement(SymbolElement const & se, math::Matrix<double, 3, 3> const & m)
    : base_t(se),
      m_symbolName(se.m_symbolName),
      m_symbolRect(se.m_symbolRect)
  {
    setPivot(se.pivot() * m);
  }

  vector<m2::AnyRectD> const & SymbolElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      m_boundRects.push_back(boundRect());
      setIsDirtyRect(false);
    }
    return m_boundRects;
  }

  m2::AnyRectD const SymbolElement::boundRect() const
  {
    m2::RectI texRect(m_symbolRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), math::Identity<double, 3>());

    return m2::AnyRectD(m2::RectD(posPt, posPt + m2::PointD(texRect.SizeX(), texRect.SizeY())));
  }

  void SymbolElement::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (!isNeedRedraw())
      return;

    uint32_t styleID = r->skin()->mapSymbol(m_symbolName.c_str());
    ResourceStyle const * style = r->skin()->fromID(styleID);

    if (style == 0)
    {
      LOG(LINFO, ("POI(", m_symbolName, ") wasn't found on the current skin"));
      return;
    }

    if (style->m_texRect != m_symbolRect)
    {
      LOG(LINFO, ("POI(", m_symbolName, ") rects do not match."));
      return;
    }

    m2::RectI texRect(style->m_texRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), m);

    r->drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          yg::maxDepth,
                          style->m_pipelineID);
  }

  void SymbolElement::map(StylesCache * stylesCache) const
  {
  }

  bool SymbolElement::find(StylesCache * stylesCache) const
  {
    return true;
  }

  void SymbolElement::fillUnpacked(StylesCache * stylesCache, vector<m2::PointU> & v) const
  {
  }

  int SymbolElement::visualRank() const
  {
    return 0000;
  }

  OverlayElement * SymbolElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new SymbolElement(*this, m);
  }
}
