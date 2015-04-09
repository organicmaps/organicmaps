#pragma once

#include "graphics/overlay_element.hpp"
#include "graphics/circle.hpp"


namespace graphics
{
  class CircleElement : public OverlayElement
  {
    Circle::Info m_ci;

  public:
    typedef OverlayElement BaseT;

    struct Params : public BaseT::Params
    {
      Circle::Info m_ci;
      Params();
    };

    CircleElement(Params const & p);

    virtual m2::RectD GetBoundRect() const;

    void draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;

    void setTransformation(const math::Matrix<double, 3, 3> & m);
  };
}
