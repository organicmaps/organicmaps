#pragma once

#include "graphics/symbol_element.hpp"
#include "graphics/circle_element.hpp"

namespace graphics
{
  class CircledSymbol : public SymbolElement
  {
  public:
    CircledSymbol(SymbolElement::Params const & symbolElement,
                  CircleElement::Params const & circleElement);

    void draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;
    void setTransformation(const math::Matrix<double, 3, 3> & m);

  private:
    CircleElement m_circle;
  };
}
