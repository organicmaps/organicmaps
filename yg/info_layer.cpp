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
    if (r->isFrozen())
      return false;

    vector<m2::AARectD> const & lr = l->boundRects();
    vector<m2::AARectD> const & rr = r->boundRects();

    bool isIntersect = false;

    for (vector<m2::AARectD>::const_iterator lit = lr.begin(); lit != lr.end(); ++lit)
    {
      for (vector<m2::AARectD>::const_iterator rit = rr.begin(); rit != rr.end(); ++rit)
      {
        isIntersect = lit->IsIntersect(*rit);
        if (isIntersect)
          break;
      }
      if (isIntersect)
        break;
    }

    if (!isIntersect)
      return true;

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

  void InfoLayer::replaceOverlayElement(shared_ptr<OverlayElement> const & oe)
  {
    m_tree.ReplaceIf(oe, &betterOverlayElement);
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
    if (m_couldOverlap)
      addOverlayElement(oe);
    else
      replaceOverlayElement(oe);
  }

  void InfoLayer::merge(InfoLayer const & layer, math::Matrix<double, 3, 3> const & m)
  {
    layer.m_tree.ForEach(bind(&InfoLayer::processOverlayElement, this, _1, m));
  }

  void InfoLayer::cache(StylesCache * stylesCache)
  {
    m_tree.ForEach(bind(&OverlayElement::cache, _1, stylesCache));
  }
}

