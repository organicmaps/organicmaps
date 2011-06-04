#include "defines.hpp"
#include "text_renderer.hpp"
#include "resource_manager.hpp"
#include "skin.hpp"
#include "render_state.hpp"
#include "glyph_layout.hpp"
#include "resource_style.hpp"

#include "../geometry/angles.hpp"

#include "../std/bind.hpp"


#include "../base/string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

namespace yg
{
  namespace gl
  {
    TextRenderer::Params::Params()
      : m_textTreeAutoClean(true),
      m_useTextTree(false),
      m_drawTexts(true),
      m_doPeriodicalTextUpdate(false)
    {}

    TextRenderer::TextRenderer(Params const & params)
      : base_t(params),
      m_needTextRedraw(false),
      m_textTreeAutoClean(params.m_textTreeAutoClean),
      m_useTextTree(params.m_useTextTree),
      m_drawTexts(params.m_drawTexts),
      m_doPeriodicalTextUpdate(params.m_doPeriodicalTextUpdate)
    {}

    TextRenderer::TextObj::TextObj(StraightTextElement const & elem)
       : m_elem(elem), m_needRedraw(true), m_frozen(false)
    {
    }

    void TextRenderer::TextObj::Draw(TextRenderer * pTextRenderer) const
    {
      /// this value is assigned inside offsetTextRect function to the texts which completely
      /// lies inside the testing rect and therefore should be skipped.
      if (m_needRedraw)
      {
        m_elem.draw(pTextRenderer);
        m_frozen = true;
      }
    }

    m2::RectD const TextRenderer::TextObj::GetLimitRect(TextRenderer* pTextRenderer) const
    {
      return m_elem.boundRect();
    }

    void TextRenderer::TextObj::SetNeedRedraw(bool flag) const
    {
      m_needRedraw = flag;
    }

    bool TextRenderer::TextObj::IsNeedRedraw() const
    {
      return m_needRedraw;
    }

    bool TextRenderer::TextObj::IsFrozen() const
    {
      return m_frozen;
    }

    string const & TextRenderer::TextObj::Text() const
    {
      return m_elem.utf8Text();
    }

    void TextRenderer::TextObj::Offset(m2::PointD const & offs)
    {
      m_elem.offset(offs);
    }

    bool TextRenderer::TextObj::better_text(TextObj const & r1, TextObj const & r2)
    {
      // any text is worse than a frozen one,
      // because frozen texts shouldn't be popped out by newly arrived texts.
      if (r2.m_frozen)
        return false;
      if (r1.m_elem.fontDesc() != r2.m_elem.fontDesc())
        return r1.m_elem.fontDesc() > r2.m_elem.fontDesc();
      return (r1.m_elem.depth() > r2.m_elem.depth());
    }

    void TextRenderer::drawText(FontDesc const & fontDesc,
                                m2::PointD const & pt,
                                yg::EPosition pos,
                                float angle,
                                string const & utf8Text,
                                double depth,
                                bool log2vis)
    {
      if (!m_drawTexts)
        return;

      StraightTextElement::Params params;
      params.m_depth = depth;
      params.m_fontDesc = fontDesc;
      params.m_log2vis = log2vis;
      params.m_pivot = pt;
      params.m_position = pos;
      params.m_rm = resourceManager();
      params.m_skin = skin();
      params.m_utf8Text = utf8Text;

      StraightTextElement ste(params);

      if (!m_useTextTree || fontDesc.m_isStatic)
        ste.draw(this);
      else
      {
        checkTextRedraw();
        TextObj obj(ste);
        m2::RectD r = obj.GetLimitRect(this);
        m_tree.ReplaceIf(obj, r, &TextObj::better_text);
      }
    }

    void TextRenderer::checkTextRedraw()
    {
      ASSERT(m_useTextTree, ());
      if (m_needTextRedraw)
      {
        m_needTextRedraw = false;
        m_tree.ForEach(bind(&TextObj::Draw, _1, this));

        /// flushing only texts
        base_t::flush(skin()->currentTextPage());
      }
    }

    void TextRenderer::setClipRect(m2::RectI const & rect)
    {
      if (m_useTextTree)
      {
        bool needTextRedraw = m_needTextRedraw;
        checkTextRedraw();
        base_t::setClipRect(rect);
        setNeedTextRedraw(needTextRedraw);
      }
      else
        base_t::setClipRect(rect);
    }

    void TextRenderer::endFrame()
    {
      if (m_useTextTree)
      {
        m_tree.ForEach(bind(&TextObj::Draw, _1, this));

        unsigned pathTextDrawn = 0;
        unsigned pathTextGroups = 0;
        unsigned maxGroup = 0;

        list<string> toErase;

        for (path_text_elements::const_iterator it = m_pathTexts.begin(); it != m_pathTexts.end(); ++it)
        {
          list<PathTextElement> const & l = it->second;

          unsigned curGroup = 0;

          for (list<PathTextElement>::const_iterator j = l.begin(); j != l.end(); ++j)
          {
            j->draw(this);
            ++pathTextDrawn;
          }

          if (l.empty())
            toErase.push_back(it->first);

          ++pathTextGroups;

          if (maxGroup < l.size())
            maxGroup = l.size();

        }

        for (list<string>::const_iterator it = toErase.begin(); it != toErase.end(); ++it)
          m_pathTexts.erase(*it);

        LOG(LINFO, ("text on pathes: ", pathTextDrawn, ", groups: ", pathTextGroups, ", max group:", maxGroup));

        if (m_textTreeAutoClean)
        {
          m_tree.Clear();
          m_pathTexts.clear();
        }

        m_needTextRedraw = false;
      }
      base_t::endFrame();
    }

    void TextRenderer::clearTextTree()
    {
      ASSERT(m_useTextTree, ());
      m_tree.Clear();
      m_pathTexts.clear();
    }

    void TextRenderer::offsetTexts(m2::PointD const & offs, m2::RectD const & rect)
    {
      ASSERT(m_useTextTree, ());
      vector<TextObj> texts;
      m_tree.ForEach(MakeBackInsertFunctor(texts));
      m_tree.Clear();
      for (vector<TextObj>::iterator it = texts.begin(); it != texts.end(); ++it)
      {
        it->Offset(offs);

        m2::RectD limitRect = it->GetLimitRect(this);

        /// fully inside shouldn't be rendered
        if (rect.IsRectInside(limitRect))
          it->SetNeedRedraw(false);
        else
          /// intersecting the borders, should be re-rendered
          if (rect.IsIntersect(limitRect))
            it->SetNeedRedraw(true);

        if (limitRect.IsIntersect(rect))
          m_tree.Add(*it, limitRect);
      }
    }

    void TextRenderer::offsetPathTexts(m2::PointD const & offs, m2::RectD const & rect)
    {
      ASSERT(m_useTextTree, ());

      for (path_text_elements::iterator i = m_pathTexts.begin(); i != m_pathTexts.end(); ++i)
      {
        list<PathTextElement> & l = i->second;
        list<PathTextElement>::iterator it = l.begin();
        while (it != l.end())
        {
          it->offset(offs);
          m2::RectD const & r = it->boundRect();
          if (!rect.IsIntersect(r) && !rect.IsRectInside(r))
          {
            list<PathTextElement>::iterator tempIt = it;
            ++tempIt;
            l.erase(it);
            it = tempIt;
          }
          else
            ++it;
        }

      }
    }

    void TextRenderer::offsetTextTree(m2::PointD const & offs, m2::RectD const & rect)
    {
      offsetTexts(offs, rect);
      offsetPathTexts(offs, rect);
    }

    void TextRenderer::setNeedTextRedraw(bool flag)
    {
      ASSERT(m_useTextTree, ());
      m_needTextRedraw = flag;
    }

    void TextRenderer::updateActualTarget()
    {
      if (m_useTextTree)
        setNeedTextRedraw(m_doPeriodicalTextUpdate);
      base_t::updateActualTarget();
    }

    bool TextRenderer::drawPathText(
        FontDesc const & fontDesc, m2::PointD const * path, size_t s, string const & utf8Text,
        double fullLength, double pathOffset, yg::EPosition pos, double depth)
    {
      if (!m_drawTexts)
        return false;

      PathTextElement::Params params;

      params.m_pts = path;
      params.m_ptsCount = s;
      params.m_fullLength = fullLength;
      params.m_pathOffset = pathOffset;
      params.m_fontDesc = fontDesc;
      params.m_utf8Text = utf8Text;
      params.m_depth = depth;
      params.m_log2vis = true;
      params.m_rm = resourceManager();
      params.m_skin = skin();
      params.m_pivot = path[0];
      params.m_position = pos;

      PathTextElement pte(params);

      if (!m_useTextTree || fontDesc.m_isStatic)
         pte.draw(this);
      else
      {
        checkTextRedraw();

        list<PathTextElement> & l = m_pathTexts[utf8Text];

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

      return true;
    }

    void TextRenderer::drawGlyph(m2::PointD const & ptOrg, m2::PointD const & ptGlyph, float angle, float /*blOffset*/, CharStyle const * p, double depth)
    {
      float x0 = ptGlyph.x + (p->m_xOffset - 1);
      float y1 = ptGlyph.y - (p->m_yOffset - 1);
      float y0 = y1 - (p->m_texRect.SizeY() - 2);
      float x1 = x0 + (p->m_texRect.SizeX() - 2);

      drawTexturedPolygon(ptOrg, angle,
                          p->m_texRect.minX() + 1,
                          p->m_texRect.minY() + 1,
                          p->m_texRect.maxX() - 1,
                          p->m_texRect.maxY() - 1,
                          x0, y0, x1, y1,
                          depth,
                          p->m_pageID);
    }

    void TextRenderer::drawPath(m2::PointD const * points, size_t pointsCount, double offset, uint32_t styleID, double depth)
    {
      if (m_useTextTree)
        checkTextRedraw();
      base_t::drawPath(points, pointsCount, offset, styleID, depth);
    }
  }
}
