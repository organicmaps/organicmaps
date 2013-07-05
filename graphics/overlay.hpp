#pragma once

#include "text_element.hpp"
#include "symbol_element.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/tree4d.hpp"

#include "../base/matrix.hpp"
#include "../base/mutex.hpp"

#include "../std/map.hpp"
#include "../std/list.hpp"

namespace graphics
{
  struct OverlayElementTraits
  {
    static m2::RectD const LimitRect(shared_ptr<OverlayElement> const & elem);
  };

  class Overlay
  {
  private:

    threads::Mutex m_mutex;

    bool m_couldOverlap;

    m4::Tree<shared_ptr<OverlayElement>, OverlayElementTraits> m_tree;

    void addOverlayElement(shared_ptr<OverlayElement> const & oe);
    void replaceOverlayElement(shared_ptr<OverlayElement> const & oe);

    Overlay(Overlay const & src) {}

  public:

    Overlay();

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    void selectOverlayElements(m2::RectD const & rect, list<shared_ptr<OverlayElement> > & res) const;

    void removeOverlayElement(shared_ptr<OverlayElement> const & oe, m2::RectD const & r);

    void processOverlayElement(shared_ptr<OverlayElement> const & oe);

    void processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m);

    void offset(m2::PointD const & offs, m2::RectD const & rect);

    size_t getElementsCount() const;

    void lock();
    void unlock();

    void clear();

    void setCouldOverlap(bool flag);

    void merge(Overlay const & infoLayer, math::Matrix<double, 3, 3> const & m);
    void merge(Overlay const & infoLayer);

    void clip(m2::RectI const & r);

    bool checkHasEquals(Overlay const * l) const;

    template <typename Fn>
    void forEach(Fn fn)
    {
      m_tree.ForEach(fn);
    }
  };
}
