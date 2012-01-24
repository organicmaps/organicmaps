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
  class StylesCache;

  struct OverlayElementTraits
  {
    static m2::RectD const LimitRect(shared_ptr<OverlayElement> const & elem);
  };

  class InfoLayer
  {
  private:

    bool m_couldOverlap;

    m4::Tree<shared_ptr<OverlayElement>, OverlayElementTraits> m_tree;

    void addOverlayElement(shared_ptr<OverlayElement> const & oe);
    void replaceOverlayElement(shared_ptr<OverlayElement> const & oe);

    friend class InfoLayer;

  public:

    InfoLayer();
    InfoLayer(InfoLayer const & src);

    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    void processOverlayElement(shared_ptr<OverlayElement> const & oe);

    void processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m);

    void offset(m2::PointD const & offs, m2::RectD const & rect);

    void clear();

    void cache(StylesCache * cache);

    void setCouldOverlap(bool flag);

    void merge(InfoLayer const & infoLayer, math::Matrix<double, 3, 3> const & m);

    void clip(m2::RectI const & r);

    bool checkHasEquals(InfoLayer const * l) const;
    bool checkCached(StylesCache * s) const;

    InfoLayer * clone() const;
  };
}
