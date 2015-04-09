#include "graphics/circled_symbol.hpp"

namespace graphics
{
  CircledSymbol::CircledSymbol(SymbolElement::Params const & symbolElement,
                               CircleElement::Params const & circleElement)
    : SymbolElement(symbolElement)
    , m_circle(circleElement) {}

  void CircledSymbol::draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const
  {
    m_circle.draw(s, m);
    SymbolElement::draw(s, m);
  }

  void CircledSymbol::setTransformation(const math::Matrix<double, 3, 3> & m)
  {
    m_circle.setTransformation(m);
    SymbolElement::setTransformation(m);
  }
}
