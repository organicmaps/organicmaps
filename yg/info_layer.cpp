#include "../base/SRC_FIRST.hpp"

#include "info_layer.hpp"
#include "text_element.hpp"

#include "../std/bind.hpp"
#include "../std/vector.hpp"

namespace yg
{
  m2::RectD const StraightTextElementTraits::LimitRect(StraightTextElement const & elem)
  {
    return elem.boundRect().GetGlobalRect();
  }

  bool InfoLayer::better_text(StraightTextElement const & r1, StraightTextElement const & r2)
  {
    /// any text is worse than a frozen one
    if (r2.isFrozen())
      return false;
    if (r1.fontDesc() != r2.fontDesc())
      return r1.fontDesc() > r2.fontDesc();
    if (r1.depth() != r2.depth())
      return r1.depth() > r2.depth();
    return false;
  }

  void InfoLayer::draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m)
  {
    for (symbols_map_t::const_iterator it = m_symbolsMap.begin(); it != m_symbolsMap.end(); ++it)
      it->second.ForEach(bind(&SymbolElement::draw, _1, r, m));

    list<strings::UniString> toErase;

    for (path_text_elements::const_iterator it = m_pathTexts.begin(); it != m_pathTexts.end(); ++it)
    {
      list<PathTextElement> const & l = it->second;

      for (list<PathTextElement>::const_iterator j = l.begin(); j != l.end(); ++j)
        j->draw(r, m);

      if (l.empty())
        toErase.push_back(it->first);
    }

    for (list<strings::UniString>::const_iterator it = toErase.begin(); it != toErase.end(); ++it)
      m_pathTexts.erase(*it);

    m_tree.ForEach(bind(&StraightTextElement::draw, _1, r, m));
  }

/*  template <typename Tree>
  void offsetTree(Tree & tree, m2::PointD const & offs, m2::RectD const & rect)
  {
    typedef typename Tree::elem_t elem_t;
    vector<elem_t> elems;
    tree.ForEach(MakeBackInsertFunctor(elems));
    tree.Clear();

    for (typename vector<elem_t>::iterator it = elems.begin(); it != elems.end(); ++it)
    {
      it->offset(offs);
      m2::RectD limitRect = it->boundRect().GetGlobalRect();
      bool doAppend = false;

      it->setIsNeedRedraw(true);
      it->setIsFrozen(false);

      if (rect.IsRectInside(limitRect))
      {
        it->setIsFrozen(true);
        doAppend = true;
      }
      else
        if (rect.IsIntersect(limitRect))
        {
          it->setIsFrozen(true);
          doAppend = true;
        }

      if (doAppend)
        tree.Add(*it, limitRect);
    }
  }

  void InfoLayer::offsetTextTree(m2::PointD const & offs, m2::RectD const & rect)
  {
    offsetTree(m_tree, offs, rect);
  }

  void InfoLayer::offsetPathTexts(m2::PointD const & offs, m2::RectD const & rect)
  {
    m2::AARectD aaRect(rect);

    path_text_elements newPathTexts;

    for (path_text_elements::iterator i = m_pathTexts.begin(); i != m_pathTexts.end(); ++i)
    {
      list<PathTextElement> & l = i->second;
      list<PathTextElement>::iterator it = l.begin();
      bool isEmpty = true;
      while (it != l.end())
      {
        it->offset(offs);
        m2::AARectD const & r = it->boundRect();
        if (!aaRect.IsIntersect(r) && !aaRect.IsRectInside(r))
        {
          list<PathTextElement>::iterator tempIt = it;
          ++tempIt;
          l.erase(it);
          it = tempIt;
        }
        else
        {
          isEmpty = false;
          ++it;
        }
      }

      if (!isEmpty)
        newPathTexts[i->first] = l;
    }

    /// to clear an empty elements from the map.
    m_pathTexts = newPathTexts;
  }

  void InfoLayer::offsetSymbols(m2::PointD const & offs, m2::RectD const & r)
  {
    for (symbols_map_t::iterator it = m_symbolsMap.begin(); it != m_symbolsMap.end(); ++it)
      offsetTree(it->second, offs, r);
  }

  void InfoLayer::offset(m2::PointD const & offs, m2::RectD const & rect)
  {
    offsetTextTree(offs, rect);
    offsetPathTexts(offs, rect);
    offsetSymbols(offs, rect);
  }*/

  void InfoLayer::clear()
  {
    m_tree.Clear();
    m_pathTexts.clear();
    m_symbolsMap.clear();
  }

  void InfoLayer::addStraightTextImpl(StraightTextElement const & ste)
  {
    m_tree.ReplaceIf(ste, ste.boundRect().GetGlobalRect(), &better_text);
  }

  void InfoLayer::addStraightText(StraightTextElement const & ste, math::Matrix<double, 3, 3> const & m)
  {
    if (m == math::Identity<double, 3>())
      addStraightTextImpl(ste);
    else
      addStraightTextImpl(StraightTextElement(ste, m));
  }

  void mark_intersect(bool & flag)
  {
    flag = true;
  }

  void InfoLayer::addSymbolImpl(SymbolElement const & se)
  {
    bool isIntersect = false;
    m2::RectD limitRect = se.boundRect().GetGlobalRect();
    m_symbolsMap[se.styleID()].ForEachInRect(limitRect, bind(&mark_intersect, ref(isIntersect)));
    if (!isIntersect)
      m_symbolsMap[se.styleID()].Add(se, limitRect);
  }

  void InfoLayer::addSymbol(SymbolElement const & se, math::Matrix<double, 3, 3> const & m)
  {
    if (m == math::Identity<double, 3>())
      addSymbolImpl(se);
    else
      addSymbolImpl(SymbolElement(se, m));
  }

  void InfoLayer::addPathTextImpl(PathTextElement const & pte)
  {
    list<PathTextElement> & l = m_pathTexts[pte.logText()];

    bool doAppend = true;

    for (list<PathTextElement>::const_iterator it = l.begin(); it != l.end(); ++it)
      if (it->boundRect().IsIntersect(pte.boundRect()))
      {
        doAppend = false;
        break;
      }

    if (doAppend)
      l.push_back(pte);
  }

  void InfoLayer::addPathText(PathTextElement const & pte, math::Matrix<double, 3, 3> const & m)
  {
/*    if (m == math::Identity<double, 3>())
      addPathTextImpl(pte);
    else
      addPathTextImpl(PathTextElement(pte, m));*/
  }

  void InfoLayer::merge(InfoLayer const & layer, math::Matrix<double, 3, 3> const & m)
  {
    for (symbols_map_t::const_iterator it = layer.m_symbolsMap.begin(); it != layer.m_symbolsMap.end(); ++it)
      it->second.ForEach(bind(&InfoLayer::addSymbol, this, _1, m));

    layer.m_tree.ForEach(bind(&InfoLayer::addStraightText, this, _1, m));

    for (path_text_elements::const_iterator it = layer.m_pathTexts.begin(); it != layer.m_pathTexts.end(); ++it)
      for_each(it->second.begin(), it->second.end(), bind(&InfoLayer::addPathText, this, _1, m));
  }
}

