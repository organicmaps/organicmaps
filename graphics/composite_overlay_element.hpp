#pragma once

#include "overlay_element.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

namespace graphics
{
  /*class CompositeOverlayElement : public OverlayElement
  {
  private:

    vector<shared_ptr<OverlayElement> > m_elements;

    mutable vector<m2::AnyRectD> m_boundRects;

  public:

    CompositeOverlayElement(OverlayElement::Params const & p);

    void addElement(shared_ptr<OverlayElement> const & e);

    OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    double priority() const;

    void offset(m2::PointD const & offs);
  };*/
}
