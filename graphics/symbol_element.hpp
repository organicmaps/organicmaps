#pragma once

#include "overlay_element.hpp"
#include "icon.hpp"

namespace graphics
{
  class Skin;

  class SymbolElement : public OverlayElement
  {
  private:

    Icon::Info m_info;
    m2::RectU m_symbolRect;

    m2::AnyRectD const boundRect() const;

  protected:
    mutable vector<m2::AnyRectD> m_boundRects;

  public:

    typedef OverlayElement base_t;

    struct Params : public base_t::Params
    {
      Icon::Info m_info;
      m2::RectU m_symbolRect;
      OverlayRenderer * m_renderer;
      Params();
    };

    SymbolElement(Params const & p);

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;

    uint32_t resID() const;

    bool hasSharpGeometry() const;

    void setTransformation(const math::Matrix<double, 3, 3> & m);
  };
}
