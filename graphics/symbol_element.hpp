#pragma once

#include "graphics/overlay_element.hpp"
#include "graphics/icon.hpp"

namespace graphics
{
  class Skin;

  class SymbolElement : public OverlayElement
  {
    Icon::Info m_info;
    m2::RectU m_symbolRect;

  public:
    typedef OverlayElement BaseT;

    struct Params : public BaseT::Params
    {
      Icon::Info m_info;
      m2::RectU m_symbolRect;
      OverlayRenderer * m_renderer;
      Params();
    };

    SymbolElement(Params const & p);

    virtual m2::RectD GetBoundRect() const;

    void draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;

    uint32_t resID() const;

    bool hasSharpGeometry() const;

    void setTransformation(const math::Matrix<double, 3, 3> & m);
  };
}
