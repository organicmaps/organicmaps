#pragma once

#include "symbol_element.hpp"
#include "circle_element.hpp"

namespace graphics
{
  class CircledSymbol : public SymbolElement
  {
  public:
    CircledSymbol(SymbolElement::Params const & symbolElement,
                  CircleElement::Params const & circleElement);

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;
    void setTransformation(const math::Matrix<double, 3, 3> & m);

  private:
    CircleElement m_circle;
  };
}
