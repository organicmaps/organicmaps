#include "circle_element.hpp"

#include "overlay_renderer.hpp"

namespace graphics
{
  CircleElement::CircleElement(Params const & p)
    : base_t(p),
      m_ci(p.m_ci)
  {}

  CircleElement::CircleElement(CircleElement const & ce, math::Matrix<double, 3, 3> const & m)
    : base_t(ce),
      m_ci(ce.m_ci)
  {
    setPivot(ce.pivot() * m);
  }

  vector<m2::AnyRectD> const & CircleElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      m_boundRects.push_back(boundRect());
      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  m2::AnyRectD const CircleElement::boundRect() const
  {
    m2::RectI texRect(m2::PointI(0, 0),
                      m2::PointI(m_ci.resourceSize()));

    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), math::Identity<double, 3>());

    return m2::AnyRectD(m2::RectD(posPt, posPt + m2::PointD(texRect.SizeX(), texRect.SizeY())));
  }

  void CircleElement::draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (!isNeedRedraw())
      return;

    uint32_t resID = r->mapInfo(m_ci);

    Resource const * res = r->fromID(resID);
    ASSERT_NOT_EQUAL ( res, 0, () );

    m2::RectI texRect(res->m_texRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), m);

    r->drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          graphics::maxDepth,
                          res->m_pipelineID);
  }

  OverlayElement * CircleElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new CircleElement(*this, m);
  }
}
