#pragma once

#include "overlay_element.hpp"

namespace yg
{
  struct ResourceStyle;
  class Skin;

  class SymbolElement : public OverlayElement
  {
  private:

    mutable uint32_t m_styleID;
    mutable ResourceStyle const * m_style;
    string m_symbolName;
    mutable m2::RectU m_symbolRect;

  public:

    struct Params : public OverlayElement::Params
    {
      Skin * m_skin;
      string m_symbolName;
      uint32_t m_styleID;
    };

    SymbolElement(Params const & p);
    SymbolElement(SymbolElement const & se, math::Matrix<double, 3, 3> const & m);

    m2::AARectD const boundRect() const;
    void draw(gl::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;

    uint32_t styleID() const;
  };
}
