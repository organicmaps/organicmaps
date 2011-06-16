#pragma once

#include "text_element.hpp"
#include "symbol_element.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/tree4d.hpp"

#include "../std/map.hpp"
#include "../std/list.hpp"

namespace yg
{
  namespace gl
  {
    class TextRenderer;
  }

  struct StraightTextElementTraits
  {
    static m2::RectD const LimitRect(StraightTextElement const & elem);
  };

  class InfoLayer
  {
  private:

    static bool better_text(StraightTextElement const & r1, StraightTextElement const & r2);

    m4::Tree<StraightTextElement, StraightTextElementTraits> m_tree;
    typedef map<strings::UniString, list<PathTextElement> > path_text_elements;
    path_text_elements m_pathTexts;

    typedef map<uint32_t, m4::Tree<SymbolElement> > symbols_map_t;
    symbols_map_t m_symbolsMap;

    void offsetPathTexts(m2::PointD const & offs, m2::RectD const & rect);
    void offsetTextTree(m2::PointD const & offs, m2::RectD const & rect);
    void offsetSymbols(m2::PointD const & offs, m2::RectD const & rect);

  public:

    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    void addPathText(PathTextElement const & pte);

    void addStraightText(StraightTextElement const & ste);

    void addSymbol(SymbolElement const & se);

    void offset(m2::PointD const & offs, m2::RectD const & rect);

    void clear();
  };
}
