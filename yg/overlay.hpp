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
  struct OverlayElementTraits
  {
    static m2::RectD const LimitRect(shared_ptr<OverlayElement> const & elem);
  };

  class Overlay
  {
  private:

    bool m_couldOverlap;

    m4::Tree<shared_ptr<OverlayElement>, OverlayElementTraits> m_tree;

    void addOverlayElement(shared_ptr<OverlayElement> const & oe);
    void replaceOverlayElement(shared_ptr<OverlayElement> const & oe);

  public:

    Overlay();
    Overlay(Overlay const & src);

    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    void selectOverlayElements(m2::PointD const & pt, list<shared_ptr<OverlayElement> > & res);

    void removeOverlayElement(shared_ptr<OverlayElement> const & oe);

    void processOverlayElement(shared_ptr<OverlayElement> const & oe);

    void processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m);

    void offset(m2::PointD const & offs, m2::RectD const & rect);

    void clear();

    void setCouldOverlap(bool flag);

    void merge(Overlay const & infoLayer, math::Matrix<double, 3, 3> const & m);

    void clip(m2::RectI const & r);

    bool checkHasEquals(Overlay const * l) const;

    template <typename Fn>
    void forEach(Fn fn)
    {
      m_tree.ForEach(fn);
    }

    Overlay * clone() const;
  };
}
