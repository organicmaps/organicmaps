#include "graphics/circle_element.hpp"

#include "graphics/overlay_renderer.hpp"

namespace graphics
{
  CircleElement::Params::Params()
    : m_ci()
  {}

  CircleElement::CircleElement(Params const & p)
    : BaseT(p),
      m_ci(p.m_ci)
  {}

  m2::RectD CircleElement::GetBoundRect() const
  {
    // Skip for one pixel on every border.
    m2::PointD sz = m_ci.resourceSize();
    sz -= m2::PointD(2, 2);

    m2::PointD const posPt = computeTopLeft(sz, pivot(), position());
    return m2::RectD(posPt, posPt + sz);
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

    m2::PointD posPt = computeTopLeft(m2::PointD(texRect.SizeX(), texRect.SizeY()),
                                      pivot() * m,
                                      position());


    r->drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          depth(),
                          res->m_pipelineID);
  }

  void CircleElement::setTransformation(const math::Matrix<double, 3, 3> & m)
  {
    setPivot(pivot() * getResetMatrix() * m);
    BaseT::setTransformation(m);
  }
}
