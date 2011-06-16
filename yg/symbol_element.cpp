#include "../base/SRC_FIRST.hpp"
#include "../base/logging.hpp"

#include "symbol_element.hpp"
#include "resource_style.hpp"
#include "overlay_renderer.hpp"
#include "skin.hpp"

namespace yg
{
  SymbolElement::SymbolElement(Params const & p)
    : OverlayElement(p),
      m_styleID(p.m_styleID)
  {
    m_style = p.m_skin->fromID(m_styleID);
    if (m_style == 0)
      LOG(LINFO, ("drawSymbolImpl: styleID=", m_styleID, " wasn't found on the current skin"));
  }

  m2::AARectD const SymbolElement::boundRect() const
  {
    if (m_style == 0)
      return m2::AARectD();

    m2::RectU texRect(m_style->m_texRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), math::Identity<double, 3>());

    return m2::RectD(posPt, posPt + m2::PointD(texRect.SizeX(), texRect.SizeY()));
  }

  void SymbolElement::offset(m2::PointD const & offs)
  {
    OverlayElement::offset(offs);
  }

  void SymbolElement::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (m_style == 0)
      return;
    m2::RectU texRect(m_style->m_texRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), m);

    r->drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          depth(),
                          m_style->m_pageID);
  }

  uint32_t SymbolElement::styleID() const
  {
    return m_styleID;
  }
}
