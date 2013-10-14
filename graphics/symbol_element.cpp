#include "../base/logging.hpp"

#include "symbol_element.hpp"
#include "resource.hpp"
#include "icon.hpp"
#include "overlay_renderer.hpp"

namespace graphics
{
  SymbolElement::Params::Params()
    : m_info(),
      m_symbolRect(0, 0, 0, 0),
      m_renderer(0)
  {}

  SymbolElement::SymbolElement(Params const & p)
    : base_t(p),
      m_info(p.m_info),
      m_symbolRect(0, 0, 0, 0)
  {
    uint32_t resID = p.m_renderer->findInfo(m_info);
    Resource const * res = p.m_renderer->fromID(resID);

    if (res == 0)
    {
      LOG(LWARNING, ("POI", m_info.m_name, "wasn't found on current skin."));
      return;
    }

    m_symbolRect = res->m_texRect;
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

    m2::PointD sz(texRect.SizeX(), texRect.SizeY());

    m2::PointD const posPt = computeTopLeft(sz,
                                            pivot(),
                                            position());

    return m2::AnyRectD(m2::RectD(posPt, posPt + sz));
  }

  void SymbolElement::draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (!isNeedRedraw())
      return;

    uint32_t resID = r->findInfo(m_info);
    Resource const * res = r->fromID(resID);

    if (res == 0)
    {
      LOG(LINFO, ("POI(", m_info.m_name, ") wasn't found on the current skin"));
      return;
    }

    if (res->m_texRect != m_symbolRect)
    {
      LOG(LINFO, ("POI(", m_info.m_name, ") rects doesn't match"));
      return;
    }

    m2::RectI texRect(res->m_texRect);

    m2::PointD sz(texRect.SizeX(), texRect.SizeY());

    m2::PointD posPt = computeTopLeft(sz,
                                      pivot() * m,
                                      position());

    posPt -= pivot();

    r->drawStraightTexturedPolygon(pivot(),
                                   texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                                   posPt.x, posPt.y, posPt.x + sz.x, posPt.y + sz.y,
                                   depth(),
                                   res->m_pipelineID);
  }

  void SymbolElement::setTransformation(const math::Matrix<double, 3, 3> & m)
  {
    setPivot(pivot() * getResetMatrix() * m);
    base_t::setTransformation(m);
  }

  bool SymbolElement::hasSharpGeometry() const
  {
    return true;
  }
}
