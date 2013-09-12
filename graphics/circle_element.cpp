#include "circle_element.hpp"

#include "overlay_renderer.hpp"

namespace graphics
{
  CircleElement::Params::Params()
    : m_ci()
  {}

  CircleElement::CircleElement(Params const & p)
    : base_t(p),
      m_ci(p.m_ci)
  {}

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

    m2::PointD sz(texRect.SizeX(), texRect.SizeY());

    m2::PointD posPt = computeTopLeft(sz,
                                      pivot(),
                                      position());

    return m2::AnyRectD(m2::RectD(posPt, posPt + sz));
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
    base_t::setTransformation(m);
  }
}
