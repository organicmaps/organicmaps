#include "../base/SRC_FIRST.hpp"
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
      m_styleID(p.m_styleID),
      m_symbolName(p.m_symbolName)
  {
    if (m_styleID == 0)
      m_styleID = p.m_skin->mapSymbol(m_symbolName.c_str());

    m_style = p.m_skin->fromID(m_styleID);

    if (m_style == 0)
      LOG(LINFO, ("drawSymbolImpl: styleID=", m_styleID, " wasn't found on the current skin"));
    else
      m_symbolRect = m_style->m_texRect;
  }

  SymbolElement::SymbolElement(SymbolElement const & se, math::Matrix<double, 3, 3> const & m)
    : base_t(se),
      m_styleID(0),
      m_style(0),
      m_symbolName(se.m_symbolName),
      m_symbolRect(se.m_symbolRect)
  {
    setPivot(se.pivot() * m);
  }

  vector<m2::AARectD> const & SymbolElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      m_boundRects.push_back(boundRect());
      setIsDirtyRect(false);
    }
    return m_boundRects;
  }

  m2::AARectD const SymbolElement::boundRect() const
  {
    m2::RectI texRect(m_symbolRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), math::Identity<double, 3>());

    return m2::AARectD(m2::RectD(posPt, posPt + m2::PointD(texRect.SizeX(), texRect.SizeY())));
  }

  void SymbolElement::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (!isNeedRedraw())
      return;

    if (m_styleID == 0)
    {
      m_styleID = r->skin()->mapSymbol(m_symbolName.c_str());
      m_style = r->skin()->fromID(m_styleID);
      m_symbolRect = m_style->m_texRect;
    }

    if (m_style == 0)
      return;

    m2::RectI texRect(m_symbolRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), m);

    r->drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          yg::maxDepth,
                          m_style->m_pipelineID);
  }

  uint32_t SymbolElement::styleID() const
  {
    return m_styleID;
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
