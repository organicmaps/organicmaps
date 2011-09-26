#include "../base/SRC_FIRST.hpp"

#include "composite_overlay_element.hpp"
#include "overlay_renderer.hpp"

namespace yg
{
  CompositeOverlayElement::CompositeOverlayElement(OverlayElement::Params const & p)
    : OverlayElement(p)
  {}

  void CompositeOverlayElement::addElement(shared_ptr<OverlayElement> const & e)
  {
    m_elements.push_back(e);
    setIsDirtyRect(true);
  }

  OverlayElement * CompositeOverlayElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    CompositeOverlayElement * res = new CompositeOverlayElement(*this);
    res->m_elements.clear();
    for (unsigned i = 0; i < m_elements.size(); ++i)
      res->addElement(make_shared_ptr<OverlayElement>(m_elements[i]->clone(m)));
    return res;
  }

  vector<m2::AARectD> const & CompositeOverlayElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();

      for (unsigned i = 0; i < m_elements.size(); ++i)
        copy(m_elements[i]->boundRects().begin(),
             m_elements[i]->boundRects().end(),
             back_inserter(m_boundRects));

      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  void CompositeOverlayElement::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    r->drawRectangle(roughBoundRect(), yg::Color(255, 255, 0, 64), depth() - 3);
    for (unsigned i = 0; i < m_elements.size(); ++i)
      m_elements[i]->draw(r, m);
  }

  void CompositeOverlayElement::map(StylesCache * stylesCache) const
  {
    for (unsigned i = 0; i < m_elements.size(); ++i)
      m_elements[i]->map(stylesCache);
  }

  void CompositeOverlayElement::fillUnpacked(StylesCache * stylesCache, vector<m2::PointU> & v) const
  {
    for (unsigned i = 0; i < m_elements.size(); ++i)
      m_elements[i]->fillUnpacked(stylesCache, v);
  }

  bool CompositeOverlayElement::find(StylesCache * stylesCache) const
  {
    for (unsigned i = 0; i < m_elements.size(); ++i)
      if (!m_elements[i]->find(stylesCache))
        return false;

    return true;
  }

  int CompositeOverlayElement::visualRank() const
  {
    int res = numeric_limits<int>::min();

    for (unsigned i = 0; i < m_elements.size(); ++i)
      res = max(res, m_elements[i]->visualRank());

    return res;
  }

  void CompositeOverlayElement::offset(m2::PointD const & offs)
  {
    for (unsigned i = 0; i < m_elements.size(); ++i)
      m_elements[i]->offset(offs);

    setIsDirtyRect(true);
  }
}
