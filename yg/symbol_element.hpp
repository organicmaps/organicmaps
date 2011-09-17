#pragma once

#include "overlay_element.hpp"
#include "text_element.hpp"

namespace yg
{
  struct ResourceStyle;
  class Skin;
  class StylesCache;

  class SymbolElement : public StraightTextElement
  {
  private:

    mutable uint32_t m_styleID;
    mutable ResourceStyle const * m_style;
    string m_symbolName;
    mutable m2::RectU m_symbolRect;

    mutable vector<m2::AARectD> m_boundRects;

    m2::AARectD const boundRect() const;

  public:

    typedef StraightTextElement base_t;

    struct Params : public base_t::Params
    {
      Skin * m_skin;
      string m_symbolName;
      uint32_t m_styleID;
    };

    SymbolElement(Params const & p);
    SymbolElement(SymbolElement const & se, math::Matrix<double, 3, 3> const & m);

    vector<m2::AARectD> const & boundRects() const;
    void draw(gl::OverlayRenderer * s, math::Matrix<double, 3, 3> const & m) const;
    void cache(StylesCache * stylesCache) const;

    uint32_t styleID() const;

    int visualRank() const;

    OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
  };
}
