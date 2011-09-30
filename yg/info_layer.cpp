#include "../base/SRC_FIRST.hpp"

#include "info_layer.hpp"
#include "text_element.hpp"
#include "styles_cache.hpp"

#include "../std/bind.hpp"
#include "../std/vector.hpp"

#include "../base/logging.hpp"

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

  void InfoLayer::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m)
  {
    m_tree.ForEach(bind(&OverlayElement::draw, _1, r, m));
  }

  InfoLayer::InfoLayer()
    : m_couldOverlap(true)
  {}

  void InfoLayer::setCouldOverlap(bool flag)
  {
    m_couldOverlap = flag;
  }

  template <typename Tree>
  void offsetTree(Tree & tree, m2::PointD const & offs, m2::RectD const & r)
  {
    m2::AARectD aaRect(r);
    typedef typename Tree::elem_t elem_t;
    vector<elem_t> elems;
    tree.ForEach(MakeBackInsertFunctor(elems));
    tree.Clear();

    for (typename vector<elem_t>::iterator it = elems.begin(); it != elems.end(); ++it)
    {
      (*it)->offset(offs);
      vector<m2::AARectD> const & aaLimitRects = (*it)->boundRects();
      bool doAppend = false;

      (*it)->setIsNeedRedraw(false);
      (*it)->setIsFrozen(false);

      for (int i = 0; i < aaLimitRects.size(); ++i)
      {
        if (aaRect.IsRectInside(aaLimitRects[i]))
        {
          (*it)->setIsNeedRedraw(false);
          (*it)->setIsFrozen(true);
          doAppend = true;
          break;
        }
        else
          if (aaRect.IsIntersect(aaLimitRects[i]))
          {
            (*it)->setIsFrozen(true);
            (*it)->setIsNeedRedraw(true);
            doAppend = true;
            break;
          }
      }

      if (doAppend)
        tree.Add(*it);
    }
  }

  void InfoLayer::offset(m2::PointD const & offs, m2::RectD const & rect)
  {
    offsetTree(m_tree, offs, rect);
  }

  void InfoLayer::clear()
  {
    m_tree.Clear();
  }

  void InfoLayer::addOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    m_tree.Add(oe);
  }

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

      vector<m2::AARectD> const & lr = m_oe->boundRects();
      vector<m2::AARectD> const & rr = e->boundRects();

      for (vector<m2::AARectD>::const_iterator lit = lr.begin(); lit != lr.end(); ++lit)
      {
        for (vector<m2::AARectD>::const_iterator rit = rr.begin(); rit != rr.end(); ++rit)
        {
          *m_isIntersect = lit->IsIntersect(*rit);
          if (*m_isIntersect)
            return;
        }
      }
    }
  };

  void InfoLayer::replaceOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    bool isIntersect = false;
    DoPreciseIntersect fn(oe, &isIntersect);
    m_tree.ForEachInRect(oe->roughBoundRect(), fn);
    if (isIntersect)
      m_tree.ReplaceIf(oe, &betterOverlayElement);
    else
      m_tree.Add(oe);
  }

  void InfoLayer::processOverlayElement(shared_ptr<OverlayElement> const & oe, math::Matrix<double, 3, 3> const & m)
  {
    if (m != math::Identity<double, 3>())
      processOverlayElement(make_shared_ptr(oe->clone(m)));
    else
      processOverlayElement(oe);
  }

  void InfoLayer::processOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    if (oe->isVisible())
      if (m_couldOverlap)
        addOverlayElement(oe);
      else
        replaceOverlayElement(oe);
  }

  void InfoLayer::merge(InfoLayer const & layer, math::Matrix<double, 3, 3> const & m)
  {
    layer.m_tree.ForEach(bind(&InfoLayer::processOverlayElement, this, _1, m));
  }

  bool greater_priority(shared_ptr<OverlayElement> const & l,
                        shared_ptr<OverlayElement> const & r)
  {
    return l->visualRank() > r->visualRank();
  }

  void InfoLayer::cache(StylesCache * stylesCache)
  {
    /// collecting elements into vector sorted by visualPriority

    vector<shared_ptr<OverlayElement> > v;
    m_tree.ForEach(bind(&vector<shared_ptr<OverlayElement> >::push_back, &v, _1));

    sort(v.begin(), v.end(), &greater_priority);

    for (unsigned i = 0; i < v.size(); ++i)
      v[i]->setIsNeedRedraw(true);

    /// caching on StylesCache::m_maxPagesCount at most

    vector<m2::PointU> sizes;
    sizes.reserve(100);

    for (unsigned i = 0; i < v.size(); ++i)
      v[i]->fillUnpacked(stylesCache, sizes);

    if (stylesCache->hasRoom(&sizes[0], sizes.size()))
    {
      for (unsigned i = 0; i < v.size(); ++i)
        v[i]->map(stylesCache);
    }
    else
    {
      /// no room to cache, so clear all pages and re-cache from the beginning
      stylesCache->clear();

      int pos = 0;

      for (pos = 0; pos < v.size(); ++pos)
      {
        sizes.clear();
        v[pos]->fillUnpacked(stylesCache, sizes);
        if (stylesCache->hasRoom(&sizes[0], sizes.size()))
          v[pos]->map(stylesCache);
        else
          break;
      }

      if (v.size() - pos > 1)
        LOG(LINFO, ("making ", v.size() - pos, "elements invisible"));

      /// making all uncached elements invisible
      for (; pos < v.size(); ++pos)
        v[pos]->setIsNeedRedraw(false);
    }
  }
}

