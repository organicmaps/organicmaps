#include "overlay.hpp"
#include "overlay_renderer.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/vector.hpp"


namespace graphics
{
  bool betterOverlayElement(shared_ptr<OverlayElement> const & l,
                            shared_ptr<OverlayElement> const & r)
  {
    // "frozen" object shouldn't be popped out.
    if (r->isFrozen())
      return false;

    // for the composite elements, collected in OverlayRenderer to replace the part elements
    return l->priority() > r->priority();
  }

  m2::RectD const OverlayElementTraits::LimitRect(shared_ptr<OverlayElement> const & elem)
  {
    return elem->GetBoundRect();
  }

  void DrawIfNotCancelled(OverlayRenderer * r,
                          shared_ptr<OverlayElement> const & e,
                          math::Matrix<double, 3, 3> const & m)
  {
    if (!r->isCancelled())
      e->draw(r, m);
  }

  void Overlay::draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m)
  {
    m_tree.ForEach(bind(&DrawIfNotCancelled, r, _1, cref(m)));
  }

  Overlay::Overlay()
    : m_couldOverlap(true)
  {}

  void Overlay::setCouldOverlap(bool flag)
  {
    m_couldOverlap = flag;
  }

  template <typename Tree>
  void offsetTree(Tree & tree, m2::PointD const & offs, m2::RectD const & r)
  {
    m2::AnyRectD AnyRect(r);

    typedef typename Tree::elem_t elem_t;
    vector<elem_t> elems;

    tree.ForEach(MakeBackInsertFunctor(elems));
    tree.Clear();

    for (typename vector<elem_t>::iterator it = elems.begin(); it != elems.end(); ++it)
    {
      (*it)->offset(offs);
      OverlayElement::RectsT aaLimitRects;
      (*it)->GetMiniBoundRects(aaLimitRects);

      bool doAppend = false;

      (*it)->setIsNeedRedraw(false);
      (*it)->setIsFrozen(true);

      bool hasInside = false;
      bool hasOutside = false;

      for (size_t i = 0; i < aaLimitRects.size(); ++i)
      {
        if (AnyRect.IsRectInside(aaLimitRects[i]))
        {
          if (hasOutside)
          {
            (*it)->setIsNeedRedraw(true);
            doAppend = true;
            break;
          }
          else
          {
            hasInside = true;
            doAppend = true;
            continue;
          }
        }

        if (aaLimitRects[i].IsRectInside(AnyRect))
        {
          doAppend = true;
          break;
        }

        if (AnyRect.IsIntersect(aaLimitRects[i]))
        {
          (*it)->setIsNeedRedraw(true);
          doAppend = true;
          break;
        }

        // the last case remains here - part rect is outside big rect
        if (hasInside)
        {
          (*it)->setIsNeedRedraw(true);
          doAppend = true;
          break;
        }
        else
          hasOutside = true;
      }

      if (doAppend)
        tree.Add(*it);
    }
  }

  void Overlay::offset(m2::PointD const & offs, m2::RectD const & rect)
  {
    offsetTree(m_tree, offs, rect);
  }

  size_t Overlay::getElementsCount() const
  {
    return m_tree.GetSize();
  }

  void Overlay::lock()
  {
    m_mutex.Lock();
  }

  void Overlay::unlock()
  {
    m_mutex.Unlock();
  }

  void Overlay::clear()
  {
    m_tree.Clear();
  }

  void Overlay::addOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    m_tree.Add(oe);
  }

  struct DoPreciseSelectByPoint
  {
    m2::PointD m_pt;
    list<shared_ptr<OverlayElement> > * m_elements;

    DoPreciseSelectByPoint(m2::PointD const & pt, list<shared_ptr<OverlayElement> > * elements)
      : m_pt(pt), m_elements(elements)
    {}

    void operator()(shared_ptr<OverlayElement> const & e)
    {
      if (e->hitTest(m_pt))
        m_elements->push_back(e);
    }
  };

  struct DoPreciseSelectByRect
  {
    m2::AnyRectD m_rect;
    list<shared_ptr<OverlayElement> > * m_elements;

    DoPreciseSelectByRect(m2::RectD const & rect,
                          list<shared_ptr<OverlayElement> > * elements)
      : m_rect(rect),
        m_elements(elements)
    {}

    void operator()(shared_ptr<OverlayElement> const & e)
    {
      OverlayElement::RectsT rects;
      e->GetMiniBoundRects(rects);

      for (size_t i = 0; i < rects.size(); ++i)
      {
        if (m_rect.IsIntersect(rects[i]))
        {
          m_elements->push_back(e);
          break;
        }
      }
    }
  };

  class DoPreciseIntersect
  {
    shared_ptr<OverlayElement> m_oe;
    OverlayElement::RectsT m_rects;

    bool m_isIntersect;

  public:
    DoPreciseIntersect(shared_ptr<OverlayElement> const & oe)
      : m_oe(oe), m_isIntersect(false)
    {
      m_oe->GetMiniBoundRects(m_rects);
    }

    m2::RectD GetSearchRect() const
    {
      m2::RectD rect;
      for (size_t i = 0; i < m_rects.size(); ++i)
        rect.Add(m_rects[i].GetGlobalRect());
      return rect;
    }

    void operator()(shared_ptr<OverlayElement> const & e)
    {
      if (m_isIntersect)
        return;

      if (m_oe->m_userInfo == e->m_userInfo)
        return;

      OverlayElement::RectsT rects;
      e->GetMiniBoundRects(rects);

      for (size_t i = 0; i < m_rects.size(); ++i)
        for (size_t j = 0; j < rects.size(); ++j)
        {
          m_isIntersect = m_rects[i].IsIntersect(rects[j]);
          if (m_isIntersect)
            return;
        }
    }

    bool IsIntersect() const { return m_isIntersect; }
  };

  void Overlay::selectOverlayElements(m2::RectD const & rect, list<shared_ptr<OverlayElement> > & res) const
  {
    DoPreciseSelectByRect fn(rect, &res);
    m_tree.ForEachInRect(rect, fn);
  }

  void Overlay::replaceOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    DoPreciseIntersect fn(oe);
    m_tree.ForEachInRect(fn.GetSearchRect(), bind<void>(ref(fn), _1));

    if (fn.IsIntersect())
      m_tree.ReplaceIf(oe, &betterOverlayElement);
    else
      m_tree.Add(oe);
  }

  void Overlay::removeOverlayElement(shared_ptr<OverlayElement> const & oe, m2::RectD const & r)
  {
    m_tree.Erase(oe, r);
  }

  void Overlay::processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m)
  {
    oe->setTransformation(m);
    if (oe->isValid())
      processOverlayElement(oe);
  }

  void Overlay::processOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    if (oe->isValid())
    {
      if (m_couldOverlap)
        addOverlayElement(oe);
      else
        replaceOverlayElement(oe);
    }
  }

  bool greater_priority(shared_ptr<OverlayElement> const & l,
                        shared_ptr<OverlayElement> const & r)
  {
    return l->priority() > r->priority();
  }

  void Overlay::merge(Overlay const & layer, math::Matrix<double, 3, 3> const & m)
  {
    vector<shared_ptr<OverlayElement> > v;

    // 1. collecting all elements from tree
    layer.m_tree.ForEach(MakeBackInsertFunctor(v));

    // 2. sorting by priority, so the more important ones comes first
    sort(v.begin(), v.end(), &greater_priority);

    // 3. merging them into the infoLayer starting from most
    // important one to optimize the space usage.
    for_each(v.begin(), v.end(), [&] (shared_ptr<OverlayElement> const & p)
    {
      processOverlayElement(p, m);
    });
  }

  void Overlay::merge(Overlay const & infoLayer)
  {
    vector<shared_ptr<OverlayElement> > v;

    // 1. collecting all elements from tree
    infoLayer.m_tree.ForEach(MakeBackInsertFunctor(v));

    // 2. sorting by priority, so the more important ones comes first
    sort(v.begin(), v.end(), &greater_priority);

    // 3. merging them into the infoLayer starting from most
    // important one to optimize the space usage.
    for_each(v.begin(), v.end(), [this] (shared_ptr<OverlayElement> const & p)
    {
      processOverlayElement(p);
    });
  }

  void Overlay::clip(m2::RectI const & r)
  {
    vector<shared_ptr<OverlayElement> > v;
    m_tree.ForEach(MakeBackInsertFunctor(v));
    m_tree.Clear();

    m2::RectD const rd(r);
    m2::AnyRectD ard(rd);

    for (size_t i = 0; i < v.size(); ++i)
    {
      shared_ptr<OverlayElement> const & e = v[i];

      if (!e->isVisible())
        continue;

      OverlayElement::RectsT rects;
      e->GetMiniBoundRects(rects);

      for (size_t j = 0; j < rects.size(); ++j)
        if (ard.IsIntersect(rects[j]))
        {
          processOverlayElement(e);
          break;
        }
    }
  }
}
