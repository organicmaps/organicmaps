#include "../base/SRC_FIRST.hpp"

#include "overlay.hpp"
#include "text_element.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/vector.hpp"


namespace yg
{
  bool betterOverlayElement(shared_ptr<OverlayElement> const & l,
                            shared_ptr<OverlayElement> const & r)
  {
    /// "frozen" object shouldn't be popped out.
    if (r->isFrozen())
      return false;

    /// for the composite elements, collected in OverlayRenderer to replace the part elements
    return l->visualRank() >= r->visualRank();
  }

  m2::RectD const OverlayElementTraits::LimitRect(shared_ptr<OverlayElement> const & elem)
  {
    return elem->roughBoundRect();
  }

  void Overlay::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m)
  {
    m_tree.ForEach(bind(&OverlayElement::draw, _1, r, cref(m)));
  }

  Overlay::Overlay()
    : m_couldOverlap(true)
  {}

  Overlay::Overlay(Overlay const & src)
  {
    m_couldOverlap = src.m_couldOverlap;

    vector<shared_ptr<OverlayElement> > elems;
    src.m_tree.ForEach(MakeBackInsertFunctor(elems));

    math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

    for (unsigned i = 0; i < elems.size(); ++i)
    {
      shared_ptr<OverlayElement> e(elems[i]->clone(id));

      e->setIsVisible(true);
      e->setIsNeedRedraw(true);

      processOverlayElement(e);
    }
  }

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

  void Overlay::clear()
  {
    m_tree.Clear();
  }

  void Overlay::addOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    m_tree.Add(oe);
  }

  struct DoPreciseSelect
  {
    m2::PointD m_pt;
    list<shared_ptr<OverlayElement> > * m_elements;

    DoPreciseSelect(m2::PointD const & pt, list<shared_ptr<OverlayElement> > * elements)
      : m_pt(pt), m_elements(elements)
    {}

    void operator()(shared_ptr<OverlayElement> const & e)
    {
      if (e->hitTest(m_pt))
        m_elements->push_back(e);
    }
  };

  struct DoPreciseIntersect
  {
    shared_ptr<OverlayElement> m_oe;
    bool * m_isIntersect;

    DoPreciseIntersect(shared_ptr<OverlayElement> const & oe, bool * isIntersect)
      : m_oe(oe),
        m_isIntersect(isIntersect)
    {}

    void operator()(shared_ptr<OverlayElement> const & e)
    {
      if (*m_isIntersect)
        return;

      vector<m2::AnyRectD> const & lr = m_oe->boundRects();
      vector<m2::AnyRectD> const & rr = e->boundRects();

      for (vector<m2::AnyRectD>::const_iterator lit = lr.begin(); lit != lr.end(); ++lit)
      {
        for (vector<m2::AnyRectD>::const_iterator rit = rr.begin(); rit != rr.end(); ++rit)
        {
          *m_isIntersect = lit->IsIntersect(*rit);
          if (*m_isIntersect)
            return;
        }
      }
    }
  };

  void Overlay::selectOverlayElements(m2::PointD const & pt, list<shared_ptr<OverlayElement> > & res)
  {
    DoPreciseSelect fn(pt, &res);
    m_tree.ForEachInRect(m2::RectD(pt - m2::PointD(1, 1), pt + m2::PointD(1, 1)), fn);
  }

  void Overlay::replaceOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    bool isIntersect = false;
    DoPreciseIntersect fn(oe, &isIntersect);
    m_tree.ForEachInRect(oe->roughBoundRect(), fn);
    if (isIntersect)
      m_tree.ReplaceIf(oe, &betterOverlayElement);
    else
      m_tree.Add(oe);
  }

  void Overlay::removeOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    m_tree.Erase(oe);
  }

  void Overlay::processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m)
  {
    if (m != math::Identity<double, 3>())
      processOverlayElement(make_shared_ptr(oe->clone(m)));
    else
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
    return l->visualRank() > r->visualRank();
  }

  void Overlay::merge(Overlay const & layer, math::Matrix<double, 3, 3> const & m)
  {
    vector<shared_ptr<OverlayElement> > v;

    /// 1. collecting all elements from tree
    layer.m_tree.ForEach(MakeBackInsertFunctor(v));

    /// 2. sorting by priority, so the more important ones comes first
    sort(v.begin(), v.end(), &greater_priority);

    /// 3. merging them into the infoLayer starting from most
    /// important one to optimize the space usage.
    for_each(v.begin(), v.end(), bind(&Overlay::processOverlayElement, this, _1, cref(m)));
  }

  void Overlay::clip(m2::RectI const & r)
  {
    vector<shared_ptr<OverlayElement> > v;
    m_tree.ForEach(MakeBackInsertFunctor(v));
    m_tree.Clear();

    int clippedCnt = 0;
    //int elemCnt = v.size();

    m2::RectD rd(r);
    m2::AnyRectD ard(rd);

    for (unsigned i = 0; i < v.size(); ++i)
    {
      shared_ptr<OverlayElement> const & e = v[i];

      if (!e->isVisible())
      {
        clippedCnt++;
        continue;
      }

      if (!e->roughBoundRect().IsIntersect(rd))
        clippedCnt++;
      else
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
    }

//    LOG(LINFO, ("clipped out", clippedCnt, "elements,", elemCnt, "elements total"));
  }

  bool Overlay::checkHasEquals(Overlay const * l) const
  {
    vector<shared_ptr<OverlayElement> > v0;
    m_tree.ForEach(MakeBackInsertFunctor(v0));

    sort(v0.begin(), v0.end());

    vector<shared_ptr<OverlayElement> > v1;
    l->m_tree.ForEach(MakeBackInsertFunctor(v1));

    sort(v1.begin(), v1.end());

    vector<shared_ptr<OverlayElement> > res;

    set_intersection(v0.begin(), v0.end(), v1.begin(), v1.end(), back_inserter(res));

    return !res.empty();
  }

  Overlay * Overlay::clone() const
  {
    Overlay * res = new Overlay(*this);
    return res;
  }
}

