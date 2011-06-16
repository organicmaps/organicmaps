#pragma once

#include "overlay_element.hpp"

namespace yg
{
  struct ResourceStyle;
  class Skin;

  class SymbolElement : public OverlayElement
  {
  private:

    uint32_t m_styleID;
    ResourceStyle const * m_style;

  public:

    struct Params : public OverlayElement::Params
    {
      Skin * m_skin;
      uint32_t m_styleID;
    };

    SymbolElement(Params const & p);

    m2::AARectD const boundRect() const;
    void draw(gl::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;
    void offset(m2::PointD const & offs);

    uint32_t styleID() const;
  };
}
