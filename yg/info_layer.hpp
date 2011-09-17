#pragma once

#include "text_element.hpp"
#include "symbol_element.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/tree4d.hpp"

#include "../base/matrix.hpp"

#include "../std/map.hpp"
#include "../std/list.hpp"

namespace yg
{
  namespace gl
  {
    class TextRenderer;
    class SkinPage;
  }

  class StylesCache;

  struct StraightTextElementTraits
  {
    static m2::RectD const LimitRect(StraightTextElement const & elem);
  };

  struct OverlayElementTraits
  {
    static m2::RectD const LimitRect(shared_ptr<OverlayElement> const & elem);
  };

  class InfoLayer
  {
  private:

    bool m_couldOverlap;

    static bool better_text(StraightTextElement const & r1, StraightTextElement const & r2);

    m4::Tree<shared_ptr<OverlayElement>, OverlayElementTraits> m_tree;

    m4::Tree<StraightTextElement, StraightTextElementTraits> m_textTree;
    typedef map<strings::UniString, list<PathTextElement> > path_text_elements;
    path_text_elements m_pathTexts;

    typedef map<uint32_t, m4::Tree<SymbolElement> > symbols_map_t;
    symbols_map_t m_symbolsMap;

    void offsetPathTexts(m2::PointD const & offs, m2::RectD const & rect);
    void offsetTextTree(m2::PointD const & offs, m2::RectD const & rect);
    void offsetSymbols(m2::PointD const & offs, m2::RectD const & rect);

    void addPathTextImpl(PathTextElement const & pte);
    void addStraightTextImpl(StraightTextElement const & ste);
    void addSymbolImpl(SymbolElement const & se);

    void addOverlayElementImpl(shared_ptr<OverlayElement> const & oe);
    void replaceOverlayElementImpl(shared_ptr<OverlayElement> const & oe);

  public:

    InfoLayer();

    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    void addPathText(PathTextElement const & pte, math::Matrix<double, 3, 3> const & m);

    void addStraightText(StraightTextElement const & ste, math::Matrix<double, 3, 3> const & m);

    void addSymbol(SymbolElement const & se, math::Matrix<double, 3, 3> const & m);

    void addOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m);
    void replaceOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m);

    void offset(m2::PointD const & offs, m2::RectD const & rect);

    void clear();

    void cache(StylesCache * cache);

    void setCouldOverlap(bool flag);

    void merge(InfoLayer const & infoLayer, math::Matrix<double, 3, 3> const & m);
  };
}
