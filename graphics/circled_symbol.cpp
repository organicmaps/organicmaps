#include "circled_symbol.hpp"

namespace graphics
{
  CircledSymbol::CircledSymbol(SymbolElement::Params const & symbolElement,
                               CircleElement::Params const & circleElement)
    : SymbolElement(symbolElement)
    , m_circle(circleElement) {}

  vector<m2::AnyRectD> const & CircledSymbol::boundRects() const
  {
    if (isDirtyRect())
    {
      SymbolElement::boundRects();
      vector<m2::AnyRectD> circleBounds = m_circle.boundRects();
      m_boundRects.insert(m_boundRects.end(), circleBounds.begin(), circleBounds.end());
    }

    return SymbolElement::boundRects();
  }

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
