#include "../base/logging.hpp"

#include "symbol_element.hpp"
#include "resource.hpp"
#include "icon.hpp"
#include "overlay_renderer.hpp"

namespace graphics
{
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

  SymbolElement::SymbolElement(SymbolElement const & se, math::Matrix<double, 3, 3> const & m)
    : base_t(se),
      m_info(se.m_info),
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
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), m);

    posPt -= pivot();

    r->drawStraightTexturedPolygon(pivot(),
                                   texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                                   posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                                   depth(),
                                   res->m_pipelineID);
  }

  OverlayElement * SymbolElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new SymbolElement(*this, m);
  }

  bool SymbolElement::hasSharpGeometry() const
  {
    return true;
  }
}
