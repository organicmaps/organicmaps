#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/matrix.hpp"
#include "base/mutex.hpp"
#include "base/buffer_vector.hpp"

#include "std/list.hpp"
#include "std/shared_ptr.hpp"


namespace graphics
{
  class OverlayRenderer;
  class OverlayElement;

  struct OverlayElementTraits
  {
    static m2::RectD const LimitRect(shared_ptr<OverlayElement> const & elem);
  };

  class OverlayStorage
  {
  public:
    OverlayStorage();
    OverlayStorage(m2::RectD const & clipRect);

    size_t GetSize() const;
    void AddElement(shared_ptr<OverlayElement> const & elem);

    template <typename Functor>
    void ForEach(Functor const & fn)
    {
      for (shared_ptr<OverlayElement> const & e : m_elements)
        fn(e);
    }

  private:
    m2::AnyRectD m_clipRect;
    bool m_needClip;
    buffer_vector<shared_ptr<OverlayElement>, 128> m_elements;
  };

  class Overlay
  {
  private:
    threads::Mutex m_mutex;

    m4::Tree<shared_ptr<OverlayElement>, OverlayElementTraits> m_tree;

    void replaceOverlayElement(shared_ptr<OverlayElement> const & oe);
    void processOverlayElement(shared_ptr<OverlayElement> const & oe);
    void processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m);

    Overlay(Overlay const & src) {}

  public:
    Overlay() {}

    void selectOverlayElements(m2::RectD const & rect, list<shared_ptr<OverlayElement> > & res) const;
    size_t getElementsCount() const;

    void lock();
    void unlock();

    void clear();

    void merge(shared_ptr<OverlayStorage> const & infoLayer, math::Matrix<double, 3, 3> const & m);
    void merge(shared_ptr<OverlayStorage> const & infoLayer);

    void clip(m2::RectI const & r);

    template <typename Fn>
    void forEach(Fn fn)
    {
      m_tree.ForEach(fn);
    }
  };
}
