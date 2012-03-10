#include "circle_element.hpp"

#include "resource_style_cache.hpp"
#include "overlay_renderer.hpp"
#include "resource_style.hpp"
#include "skin.hpp"

namespace yg
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
    m2::RectI texRect(m2::PointI(0, 0), m2::PointI(m_ci.patternSize()));
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), math::Identity<double, 3>());

    return m2::AnyRectD(m2::RectD(posPt, posPt + m2::PointD(texRect.SizeX(), texRect.SizeY())));
  }

  void CircleElement::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (!isNeedRedraw())
      return;

    uint32_t styleID = r->skin()->mapCircleInfo(m_ci);

    ResourceStyle const * style = r->skin()->fromID(styleID);
    ASSERT_NOT_EQUAL ( style, 0, () );

    m2::RectI texRect(style->m_texRect);
    texRect.Inflate(-1, -1);

    m2::PointD posPt = tieRect(m2::RectD(texRect), m);

    r->drawTexturedPolygon(m2::PointD(0.0, 0.0), 0.0,
                          texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                          posPt.x, posPt.y, posPt.x + texRect.SizeX(), posPt.y + texRect.SizeY(),
                          yg::maxDepth,
                          style->m_pipelineID);
  }

  int CircleElement::visualRank() const
  {
    return 500;
  }

  void CircleElement::map(ResourceStyleCache * stylesCache) const
  {
    shared_ptr<SkinPage> const & skinPage = stylesCache->cachePage();

    ASSERT(skinPage->hasRoom(m_ci), ());

    skinPage->mapCircleInfo(m_ci);
  }

  bool CircleElement::find(ResourceStyleCache * stylesCache) const
  {
    shared_ptr<SkinPage> const & skinPage = stylesCache->cachePage();

    return skinPage->findCircleInfo(m_ci) != 0x00FFFFFF;
  }

  void CircleElement::getNonPackedRects(ResourceStyleCache * stylesCache, vector<m2::PointU> & v) const
  {
    shared_ptr<SkinPage> const & skinPage = stylesCache->cachePage();

    if (skinPage->findCircleInfo(m_ci) == 0x00FFFFFF)
      v.push_back(m_ci.patternSize());
  }

  OverlayElement * CircleElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new CircleElement(*this, m);
  }
}
