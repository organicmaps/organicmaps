#pragma once

#include "text_element.hpp"
#include "symbol_element.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "../geometry/tree4d.hpp"

#include "../base/matrix.hpp"
#include "../base/mutex.hpp"
#ifdef DEBUG
  #include "../base/thread.hpp"
#endif

#include "../std/list.hpp"

namespace graphics
{
  struct OverlayElementTraits
  {
    static m2::RectD const LimitRect(OverlayElement * elem);
  };

  class Overlay
  {
  private:
#ifdef DEBUG
    threads::ThreadID m_threadID;
#endif

    threads::Mutex m_mutex;

    bool m_couldOverlap;
    set<OverlayElement *> m_notProcessedElements;

    m4::Tree<OverlayElement *, OverlayElementTraits> m_tree;

    void addOverlayElement(OverlayElement * oe);
    void replaceOverlayElement(OverlayElement * oe);

    Overlay(Overlay const & src) {}

  public:

    struct Deleter
    {
      void operator()(Overlay * overlay)
      {
        DeleteOverlay(overlay);
      }

      static void DeleteOverlay(Overlay * overlay)
      {
        overlay->deleteElementsAndClear();
        delete overlay;
      }
    };

    class Lock
    {
    public:
      Lock(Overlay * overlay);
      ~Lock();

    private:
      Overlay * m_overlay;
    };

    Overlay();

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    void selectOverlayElements(m2::PointD const & pt, list<OverlayElement const * > & res);
    void selectOverlayElements(m2::RectD const & rect, list<OverlayElement const * > & res);

    void removeOverlayElement(OverlayElement * oe, m2::RectD const & r);

    void processOverlayElement(OverlayElement * oe);

    void processOverlayElement(OverlayElement * oe, math::Matrix<double, 3, 3> const & m);

    void offset(m2::PointD const & offs, m2::RectD const & rect);

    void lock();
    void unlock();

    void deleteElementsAndClear();
    void clear();

    void setCouldOverlap(bool flag);

    void merge(Overlay const & infoLayer, math::Matrix<double, 3, 3> const & m);

    void clip(m2::RectI const & r);

    void clearNotProcessed();
    void deleteNotProcessed();

#ifdef DEBUG
    void validateNotProcessed();
#endif

    bool checkHasEquals(Overlay const * l) const;

    template <typename Fn>
    void forEach(Fn fn)
    {
      m_tree.ForEach(fn);
    }

#ifdef DEBUG
    void setThreadID(threads::ThreadID id)
    {
      m_threadID = id;
    }

    bool validateTread(threads::ThreadID id)
    {
      return m_threadID == id;
    }

#endif
  };
}
