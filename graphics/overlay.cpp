#include "../base/SRC_FIRST.hpp"

#include "overlay.hpp"
#include "overlay_renderer.hpp"
#include "text_element.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/vector.hpp"


namespace graphics
{
  Overlay::Lock::Lock(Overlay * overlay)
    : m_overlay(overlay)
  {
    m_overlay->lock();
  }

  Overlay::Lock::~Lock()
  {
    m_overlay->unlock();
  }

  bool betterOverlayElement(OverlayElement * l,
                            OverlayElement * r)
  {
    /// "frozen" object shouldn't be popped out.
    if (r->isFrozen())
      return false;

    /// for the composite elements, collected in OverlayRenderer to replace the part elements
    return l->priority() > r->priority();
  }

  m2::RectD const OverlayElementTraits::LimitRect(OverlayElement * elem)
  {
    return elem->roughBoundRect();
  }

  void DrawIfNotCancelled(OverlayRenderer * r,
                          OverlayElement * e,
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
      vector<m2::AnyRectD> const & aaLimitRects = (*it)->boundRects();
      bool doAppend = false;

      (*it)->setIsNeedRedraw(false);
      (*it)->setIsFrozen(true);

      bool hasInside = false;
      bool hasOutside = false;

      for (int i = 0; i < aaLimitRects.size(); ++i)
      {
        bool isPartInsideRect = AnyRect.IsRectInside(aaLimitRects[i]);

        if (isPartInsideRect)
        {
          if (hasOutside)
          {
            /// intersecting
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

        bool isRectInsidePart = aaLimitRects[i].IsRectInside(AnyRect);

        if (isRectInsidePart)
        {
          doAppend = true;
          break;
        }

        bool isPartIntersectRect = AnyRect.IsIntersect(aaLimitRects[i]);

        if (isPartIntersectRect)
        {
          /// intersecting
          (*it)->setIsNeedRedraw(true);
          doAppend = true;
          break;
        }

        /// the last case remains here - part rect is outside big rect
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

  void Overlay::lock()
  {
    m_mutex.Lock();
  }

  void Overlay::unlock()
  {
    m_mutex.Unlock();
  }

  void Overlay::deleteElementsAndClear()
  {
    std::vector<OverlayElement *> elements;
    m_tree.ForEach(MakeBackInsertFunctor(elements));
    for_each(elements.begin(), elements.end(), DeleteFunctor());
    clear();
  }

  void Overlay::clear()
  {
    m_tree.Clear();
  }

  void Overlay::addOverlayElement(OverlayElement * oe)
  {
    m_tree.Add(oe);
  }

  struct DoPreciseSelectByPoint
  {
    m2::PointD m_pt;
    list<OverlayElement const * > & m_elements;

    DoPreciseSelectByPoint(m2::PointD const & pt, list<OverlayElement const * > & elements)
      : m_pt(pt), m_elements(elements)
    {}

    void operator()(OverlayElement * e)
    {
      if (e->hitTest(m_pt))
        m_elements.push_back(e);
    }
  };

  struct DoPreciseSelectByRect
  {
    m2::AnyRectD m_rect;
    list<OverlayElement const *> & m_elements;

    DoPreciseSelectByRect(m2::RectD const & rect,
                          list<OverlayElement const *> & elements)
      : m_rect(rect),
        m_elements(elements)
    {}

    void operator()(OverlayElement * e)
    {
      vector<m2::AnyRectD> const & rects = e->boundRects();

      for (vector<m2::AnyRectD>::const_iterator it = rects.begin();
           it != rects.end();
           ++it)
      {
        m2::AnyRectD const & rect = *it;

        if (m_rect.IsIntersect(rect))
        {
          m_elements.push_back(e);
          break;
        }
      }
    }
  };

  struct DoPreciseIntersect
  {
    OverlayElement const * m_oe;
    bool & m_isIntersect;

    DoPreciseIntersect(OverlayElement const * oe, bool & isIntersect)
      : m_oe(oe),
        m_isIntersect(isIntersect)
    {}

    void operator()(OverlayElement * e)
    {
      if (m_isIntersect)
        return;

      if (m_oe->m_userInfo == e->m_userInfo)
        return;

      vector<m2::AnyRectD> const & lr = m_oe->boundRects();
      vector<m2::AnyRectD> const & rr = e->boundRects();

      for (vector<m2::AnyRectD>::const_iterator lit = lr.begin(); lit != lr.end(); ++lit)
      {
        for (vector<m2::AnyRectD>::const_iterator rit = rr.begin(); rit != rr.end(); ++rit)
        {
          m_isIntersect = lit->IsIntersect(*rit);
          if (m_isIntersect)
            return;
        }
      }
    }
  };

  void Overlay::selectOverlayElements(m2::RectD const & rect, list<OverlayElement const *> & res)
  {
    DoPreciseSelectByRect fn(rect, res);
    m_tree.ForEachInRect(rect, fn);
  }

  void Overlay::selectOverlayElements(m2::PointD const & pt, list<OverlayElement const *> & res)
  {
    DoPreciseSelectByPoint fn(pt, res);
    m_tree.ForEachInRect(m2::RectD(pt - m2::PointD(1, 1), pt + m2::PointD(1, 1)), fn);
  }

  void Overlay::replaceOverlayElement(OverlayElement * oe)
  {
    bool isIntersect = false;
    DoPreciseIntersect fn(oe, isIntersect);
    m_tree.ForEachInRect(oe->roughBoundRect(), fn);
    if (isIntersect)
      m_tree.ReplaceIf(oe, &betterOverlayElement);
    else
      m_tree.Add(oe);
  }

  void Overlay::removeOverlayElement(OverlayElement * oe, m2::RectD const & r)
  {
    m_tree.Erase(oe, r);
  }

  void Overlay::processOverlayElement(OverlayElement * oe, math::Matrix<double, 3, 3> const & m)
  {
    oe->setTransformation(m);
    if (oe->isValid())
      processOverlayElement(oe);
  }

  void Overlay::processOverlayElement(OverlayElement * oe)
  {
    if (oe->isValid())
    {
      if (m_couldOverlap)
        addOverlayElement(oe);
      else
        replaceOverlayElement(oe);
    }
  }

  bool greater_priority(OverlayElement const * l,
                        OverlayElement const * r)
  {
    return l->priority() > r->priority();
  }

  void Overlay::merge(Overlay const & layer, math::Matrix<double, 3, 3> const & m)
  {
    vector<OverlayElement *> v;

    /// 1. collecting all elements from tree
    layer.m_tree.ForEach(MakeBackInsertFunctor(v));

    /// 2. sorting by priority, so the more important ones comes first
    sort(v.begin(), v.end(), &greater_priority);

    /// 3. merging them into the infoLayer starting from most
    /// important one to optimize the space usage.
    for_each(v.begin(), v.end(), bind(&Overlay::processOverlayElement, this, _1, cref(m)));
  }

  void Overlay::merge(Overlay const & infoLayer)
  {
    vector<OverlayElement *> v;

    /// 1. collecting all elements from tree
    infoLayer.m_tree.ForEach(MakeBackInsertFunctor(v));

    /// 2. sorting by priority, so the more important ones comes first
    sort(v.begin(), v.end(), &greater_priority);

    /// 3. merging them into the infoLayer starting from most
    /// important one to optimize the space usage.
    for_each(v.begin(), v.end(), bind(&Overlay::processOverlayElement, this, _1));
  }

  void Overlay::clip(m2::RectI const & r)
  {
    vector<OverlayElement * > v;
    m_tree.ForEach(MakeBackInsertFunctor(v));
    m_tree.Clear();

    //int clippedCnt = 0;

    m2::RectD rd(r);
    m2::AnyRectD ard(rd);

    for (unsigned i = 0; i < v.size(); ++i)
    {
      OverlayElement * e = v[i];

      if (!e->isVisible())
      {
        //clippedCnt++;
        continue;
      }

      if (e->roughBoundRect().IsIntersect(rd))
      {
        bool hasIntersection = false;
        for (unsigned j = 0; j < e->boundRects().size(); ++j)
        {
          if (ard.IsIntersect(e->boundRects()[j]))
          {
            hasIntersection = true;
            break;
          }
        }

        if (hasIntersection)
          processOverlayElement(e);
      }
      //else
      //  clippedCnt++;
    }

//    LOG(LINFO, ("clipped out", clippedCnt, "elements,", elemCnt, "elements total"));
  }

  bool Overlay::checkHasEquals(Overlay const * l) const
  {
    vector<OverlayElement *> v0;
    m_tree.ForEach(MakeBackInsertFunctor(v0));

    sort(v0.begin(), v0.end());

    vector<OverlayElement *> v1;
    l->m_tree.ForEach(MakeBackInsertFunctor(v1));

    sort(v1.begin(), v1.end());

    vector<OverlayElement *> res;

    set_intersection(v0.begin(), v0.end(), v1.begin(), v1.end(), back_inserter(res));

    return !res.empty();
  }
}

