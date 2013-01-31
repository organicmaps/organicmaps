#pragma once

#include "overlay_element.hpp"
#include "circle.hpp"

namespace graphics
{
  class CircleElement : public OverlayElement
  {
  private:

    Circle::Info m_ci;

    mutable vector<m2::AnyRectD> m_boundRects;

    m2::AnyRectD const boundRect() const;

  public:

    typedef OverlayElement base_t;

    struct Params : public base_t::Params
    {
      Circle::Info m_ci;
      Params();
    };

    CircleElement(Params const & p);
    CircleElement(CircleElement const & ce, math::Matrix<double, 3, 3> const & m);

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;

    OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
  };
}
