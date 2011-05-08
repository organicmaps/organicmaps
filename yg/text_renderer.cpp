#include "../base/SRC_FIRST.hpp"

#include "defines.hpp"
#include "text_renderer.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include "skin.hpp"
#include "render_state.hpp"
#include "glyph_layout.hpp"

#include "../coding/strutil.hpp"

#include "../geometry/angles.hpp"

#include "../std/bind.hpp"

#include "../3party/fribidi/lib/fribidi-deprecated.h"

#include "../base/logging.hpp"
#include "../base/start_mem_debug.hpp"


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

    TextRenderer::TextObj::TextObj(FontDesc const & fontDesc, m2::PointD const & pt, yg::EPosition pos, string const & txt, double d, bool log2vis)
       : m_fontDesc(fontDesc), m_pt(pt), m_pos(pos), m_utf8Text(txt), m_depth(d), m_needRedraw(true), m_frozen(false), m_log2vis(log2vis)
    {
    }

    void TextRenderer::TextObj::Draw(TextRenderer * pTextRenderer) const
    {
      /// this value is assigned inside offsetTextRect function to the texts which completely
      /// lies inside the testing rect and therefore should be skipped.
      if (m_needRedraw)
      {
        pTextRenderer->drawTextImpl(m_fontDesc, m_pt, m_pos, 0.0, m_utf8Text, yg::maxDepth, m_log2vis);
        m_frozen = true;
      }
    }

    m2::RectD const TextRenderer::TextObj::GetLimitRect(TextRenderer* pTextRenderer) const
    {
      m2::RectD limitRect = pTextRenderer->textRect(m_fontDesc, m_utf8Text, m_log2vis);

      double dx = -limitRect.SizeX() / 2;
      double dy = limitRect.SizeY() / 2;

      if (m_pos & EPosLeft)
        dx = -limitRect.SizeX();

      if (m_pos & EPosRight)
        dx = 0;

      if (m_pos & EPosUnder)
        dy = limitRect.SizeY();

      if (m_pos & EPosAbove)
        dy = 0;

      dx = ::floor(dx);
      dy = ::floor(dy);

      return m2::Offset(limitRect, m_pt + m2::PointD(dx, dy));
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
      return m_utf8Text;
    }

    void TextRenderer::TextObj::Offset(m2::PointD const & offs)
    {
      m_pt += offs;
    }

    bool TextRenderer::TextObj::better_text(TextObj const & r1, TextObj const & r2)
    {
      // any text is worse than a frozen one,
      // because frozen texts shouldn't be popped out by newly arrived texts.
      if (r2.m_frozen)
        return false;
      if (r1.m_fontDesc != r2.m_fontDesc)
        return r1.m_fontDesc > r2.m_fontDesc;
      return (r1.m_depth > r2.m_depth);
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

      if (!m_useTextTree || fontDesc.m_isStatic)
         drawTextImpl(fontDesc, pt, pos, angle, utf8Text, depth, log2vis);
      else
      {
        checkTextRedraw();
        TextObj obj(fontDesc, pt, pos, utf8Text, depth, log2vis);
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
        if (m_textTreeAutoClean)
          m_tree.Clear();

        m_needTextRedraw = false;
      }
      base_t::endFrame();
    }

    void TextRenderer::clearTextTree()
    {
      ASSERT(m_useTextTree, ());
      m_tree.Clear();
    }

    void TextRenderer::offsetTextTree(m2::PointD const & offs, m2::RectD const & rect)
    {
      ASSERT(m_useTextTree, ());
      vector<TextObj> texts;
      m_tree.ForEach(bind(&vector<TextObj>::push_back, ref(texts), _1));
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

    template <class ToDo>
    void TextRenderer::ForEachGlyph(FontDesc const & fontDesc, wstring const & text, ToDo toDo)
    {
      m2::PointD currPt(0, 0);
      for (size_t i = 0; i < text.size(); ++i)
      {
        uint32_t glyphID = skin()->mapGlyph(GlyphKey(text[i], fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color), fontDesc.m_isStatic);
        CharStyle const * p = static_cast<CharStyle const *>(skin()->fromID(glyphID));
        if (p)
        {
          toDo(currPt, p);
          currPt += m2::PointD(p->m_xAdvance, 0);
        }
      }
    }

    wstring TextRenderer::Log2Vis(wstring const & str)
    {
      size_t const count = str.size();
      wstring res;
      res.resize(count);
      FriBidiParType dir = FRIBIDI_PAR_LTR;  // requested base direction
      fribidi_log2vis(str.c_str(), count, &dir, &res[0], 0, 0, 0);
      return res;
    }

    void TextRenderer::drawTextImpl(FontDesc const & fontDesc, m2::PointD const & pt, yg::EPosition pos, float angle, string const & utf8Text, double depth, bool log2vis)
    {
      wstring text = FromUtf8(utf8Text);

      if (log2vis)
        text = Log2Vis(text);

      m2::RectD r = textRect(fontDesc, utf8Text, log2vis);

      m2::PointD orgPt(pt.x - r.SizeX() / 2, pt.y + r.SizeY() / 2);

      if (pos & EPosLeft)
        orgPt.x = pt.x - r.SizeX();

      if (pos & EPosRight)
        orgPt.x = pt.x;

      if (pos & EPosUnder)
        orgPt.y = pt.y + r.SizeY();

      if (pos & EPosAbove)
        orgPt.y = pt.y;

      orgPt.x = ::floor(orgPt.x);
      orgPt.y = ::floor(orgPt.y);

      yg::FontDesc desc = fontDesc;

      if (desc.m_isMasked)
      {
        ForEachGlyph(desc, text, bind(&TextRenderer::drawGlyph, this, cref(orgPt), _1, angle, 0, _2, depth));
        desc.m_isMasked = false;
      }

      ForEachGlyph(desc, text, bind(&TextRenderer::drawGlyph, this, cref(orgPt), _1, angle, 0, _2, depth));
    }

    m2::RectD const TextRenderer::textRect(FontDesc const & fontDesc, string const & utf8Text, bool log2vis)
    {
      if (m_useTextTree)
        checkTextRedraw();

      m2::RectD rect;
      m2::PointD pt(0, 0);

      wstring text = FromUtf8(utf8Text);
      if (log2vis)
        text = Log2Vis(text);

      for (size_t i = 0; i < text.size(); ++i)
      {
        if (fontDesc.m_isStatic)
        {
          uint32_t glyphID = skin()->mapGlyph(GlyphKey(text[i], fontDesc.m_size, fontDesc.m_isMasked, yg::Color(0, 0, 0, 0)), fontDesc.m_isStatic);
          CharStyle const * p = static_cast<CharStyle const *>(skin()->fromID(glyphID));
          if (p != 0)
          {
            rect.Add(pt);
            rect.Add(pt + m2::PointD(p->m_xOffset + p->m_texRect.SizeX() - 4, -p->m_yOffset - (int)p->m_texRect.SizeY() + 4));
            pt += m2::PointD(p->m_xAdvance, 0);
          }
        }
        else
        {
          GlyphMetrics const m = resourceManager()->getGlyphMetrics(GlyphKey(text[i], fontDesc.m_size, fontDesc.m_isMasked, yg::Color(0, 0, 0, 0)));

          rect.Add(pt);
          rect.Add(pt + m2::PointD(m.m_xOffset + m.m_width, - m.m_yOffset - m.m_height));
          pt += m2::PointD(m.m_xAdvance, 0);
        }
      }

      rect.Inflate(2, 2);

      return rect;
    }

    bool TextRenderer::drawPathText(
        FontDesc const & fontDesc, m2::PointD const * path, size_t s, string const & utf8Text,
        double fullLength, double pathOffset, yg::EPosition pos, double depth)
    {
      if (!m_drawTexts)
        return false;
      if (m_useTextTree)
        checkTextRedraw();

      yg::FontDesc desc = fontDesc;

      if (desc.m_isMasked)
      {
        if (!drawPathTextImpl(desc, path, s, utf8Text, fullLength, pathOffset, pos, depth))
          return false;
        else
          desc.m_isMasked = false;
      }
      return drawPathTextImpl(desc, path, s, utf8Text, fullLength, pathOffset, pos, depth);
    }


    bool TextRenderer::drawPathTextImpl(
        FontDesc const & fontDesc, m2::PointD const * path, size_t s, string const & utf8Text,
        double fullLength, double pathOffset, yg::EPosition pos, double depth)
    {
      wstring const text = Log2Vis(FromUtf8(utf8Text));

      GlyphLayout layout(resourceManager(), fontDesc, path, s, text, fullLength, pathOffset, pos);

      vector<GlyphLayoutElem> const & glyphs = layout.entries();

      if (layout.lastVisible() != text.size())
        return false;

/*    for (size_t i = layout.firstVisible(); i < layout.lastVisible(); ++i)
      {
        uint32_t const colorID = skin()->mapColor(yg::Color(fontDesc.m_isMasked ? 255 : 0, 0, fontDesc.m_isMasked ? 0 : 255, 255));
        ResourceStyle const * colorStyle = skin()->fromID(colorID);

        float x0 = glyphs[i].m_metrics.m_xOffset;
        float y1 = -glyphs[i].m_metrics.m_yOffset;
        float y0 = y1 - glyphs[i].m_metrics.m_height;
        float x1 = x0 + glyphs[i].m_metrics.m_width;

        drawTexturedPolygon(glyphs[i].m_pt, glyphs[i].m_angle,
                            colorStyle->m_texRect.minX() + 1,
                            colorStyle->m_texRect.minY() + 1,
                            colorStyle->m_texRect.maxX() - 1,
                            colorStyle->m_texRect.maxY() - 1,
                            x0, y0, x1, y1,
                            depth - 1,
                            colorStyle->m_pageID);

      }
*/
      for (size_t i = layout.firstVisible(); i < layout.lastVisible(); ++i)
      {
        uint32_t const glyphID = skin()->mapGlyph(GlyphKey(text[i], fontDesc.m_size, fontDesc.m_isMasked, fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color), fontDesc.m_isStatic);
        CharStyle const * charStyle = static_cast<CharStyle const *>(skin()->fromID(glyphID));

        drawGlyph(glyphs[i].m_pt, m2::PointD(0.0, 0.0), glyphs[i].m_angle, 0, charStyle, depth);
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
