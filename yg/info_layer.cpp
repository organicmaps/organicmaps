#include "../base/SRC_FIRST.hpp"

#include "info_layer.hpp"
#include "text_element.hpp"

#include "../std/bind.hpp"

namespace yg
{
  m2::RectD const StraightTextElementTraits::LimitRect(StraightTextElement const & elem)
  {
    return elem.boundRect().GetGlobalRect();
  }

  bool InfoLayer::better_text(StraightTextElement const & r1, StraightTextElement const & r2)
  {
    if (r1.fontDesc() != r2.fontDesc())
      return r1.fontDesc() > r2.fontDesc();
    if (r1.depth() != r2.depth())
      return r1.depth() > r2.depth();
    return false;
  }

  void InfoLayer::draw(gl::TextRenderer *r, math::Matrix<double, 3, 3> const & m)
  {
    m_tree.ForEach(bind(&StraightTextElement::draw, _1, r, m));

    list<string> toErase;

    for (path_text_elements::const_iterator it = m_pathTexts.begin(); it != m_pathTexts.end(); ++it)
    {
      list<PathTextElement> const & l = it->second;

      for (list<PathTextElement>::const_iterator j = l.begin(); j != l.end(); ++j)
        j->draw(r, m);

      if (l.empty())
        toErase.push_back(it->first);
    }

    for (list<string>::const_iterator it = toErase.begin(); it != toErase.end(); ++it)
      m_pathTexts.erase(*it);
  }

  void InfoLayer::offsetTextTree(m2::PointD const & offs, m2::RectD const & rect)
  {
    vector<StraightTextElement> texts;
    m_tree.ForEach(MakeBackInsertFunctor(texts));
    m_tree.Clear();
    for (vector<StraightTextElement>::iterator it = texts.begin(); it != texts.end(); ++it)
    {
      it->offset(offs);
      if (it->boundRect().GetGlobalRect().IsIntersect(rect))
        m_tree.Add(*it, it->boundRect().GetGlobalRect());
    }
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

  void InfoLayer::offset(m2::PointD const & offs, m2::RectD const & rect)
  {
    offsetTextTree(offs, rect);
    offsetPathTexts(offs, rect);
  }

  void InfoLayer::clear()
  {
    m_tree.Clear();
    m_pathTexts.clear();
  }

  void InfoLayer::addStraightText(StraightTextElement const & ste)
  {
    m_tree.ReplaceIf(ste, ste.boundRect().GetGlobalRect(), &better_text);
  }

  void InfoLayer::addPathText(PathTextElement const & pte)
  {
    list<PathTextElement> & l = m_pathTexts[pte.utf8Text()];

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
}

