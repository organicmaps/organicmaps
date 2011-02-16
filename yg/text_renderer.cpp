#include "../base/SRC_FIRST.hpp"

#include "defines.hpp"
#include "text_renderer.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include "skin.hpp"
#include "render_state.hpp"

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
      : m_textTreeAutoClean(true), m_useTextTree(false)
    {}

    TextRenderer::TextRenderer(Params const & params)
      : base_t(params),
      m_needTextRedraw(false),
      m_textTreeAutoClean(params.m_textTreeAutoClean),
      m_useTextTree(params.m_useTextTree)
    {}

    TextRenderer::TextObj::TextObj(m2::PointD const & pt, string const & txt, uint8_t sz, yg::Color const & c, bool isMasked, yg::Color const & maskColor, double d, bool isFixedFont, bool log2vis)
       : m_pt(pt), m_size(sz), m_utf8Text(txt), m_isMasked(isMasked), m_depth(d), m_needRedraw(true), m_frozen(false), m_isFixedFont(isFixedFont), m_log2vis(log2vis), m_color(c), m_maskColor(maskColor)
    {
    }

    void TextRenderer::TextObj::Draw(TextRenderer * pTextRenderer) const
    {
      /// this value is assigned inside offsetTextRect function to the texts which completely
      /// lies inside the testing rect and therefore should be skipped.
      if (m_needRedraw)
      {
        pTextRenderer->drawTextImpl(m_pt, 0.0, m_size, m_color, m_utf8Text, true, m_maskColor, yg::maxDepth, m_isFixedFont, m_log2vis);
        m_frozen = true;
      }
    }

    m2::RectD const TextRenderer::TextObj::GetLimitRect(TextRenderer* pTextRenderer) const
    {
      return m2::Offset(pTextRenderer->textRect(m_utf8Text, m_size, false, m_log2vis), m_pt);
    }

    void TextRenderer::TextObj::SetNeedRedraw(bool flag) const
    {
      m_needRedraw = flag;
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
      return (r1.m_depth > r2.m_depth);
    }

    void TextRenderer::drawText(m2::PointD const & pt,
                  float angle,
                  uint8_t fontSize,
                  yg::Color const & color,
                  string const & utf8Text,
                  bool isMasked,
                  yg::Color const & maskColor,
                  double depth,
                  bool isFixedFont,
                  bool log2vis)
    {
      if (!m_useTextTree || isFixedFont)
         drawTextImpl(pt, angle, fontSize, color, utf8Text, true, maskColor, depth, isFixedFont, log2vis);
      else
      {
        checkTextRedraw();
        TextObj obj(pt, utf8Text, fontSize, color, isMasked, maskColor, depth, isFixedFont, log2vis);
        m2::RectD r = obj.GetLimitRect(this);
        m_tree.ReplaceIf(obj, r, &TextObj::better_text);
      }
    }

    void TextRenderer::checkTextRedraw()
    {
      ASSERT(m_useTextTree, ());
      if (m_needTextRedraw)
      {
        m_tree.ForEach(bind(&TextObj::Draw, _1, this));
        /// flushing only texts
        base_t::flush(skin()->currentTextPage());
        m_needTextRedraw = false;
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
        setNeedTextRedraw(true);
      base_t::updateActualTarget();
    }

    template <class ToDo>
        void TextRenderer::ForEachGlyph(uint8_t fontSize, yg::Color const & color, wstring const & text, bool isMasked, bool isFixedFont, ToDo toDo)
    {
      m2::PointD currPt(0, 0);
      for (size_t i = 0; i < text.size(); ++i)
      {
        uint32_t glyphID = skin()->mapGlyph(GlyphKey(text[i], fontSize, isMasked, color), isFixedFont);
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

    void TextRenderer::drawTextImpl(m2::PointD const & pt, float angle, uint8_t fontSize, yg::Color const & color, string const & utf8Text, bool isMasked, yg::Color const & maskColor, double depth, bool isFixedFont, bool log2vis)
    {
      wstring text = FromUtf8(utf8Text);

      if (log2vis)
        text = Log2Vis(text);

      if (isMasked)
        ForEachGlyph(fontSize, maskColor, text, true, isFixedFont, bind(&TextRenderer::drawGlyph, this, cref(pt), _1, angle, 0, _2, depth));
      ForEachGlyph(fontSize, color, text, false, isFixedFont, bind(&TextRenderer::drawGlyph, this, cref(pt), _1, angle, 0, _2, depth));
    }

    m2::RectD const TextRenderer::textRect(string const & utf8Text, uint8_t fontSize, bool fixedFont, bool log2vis)
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
        if (fixedFont)
        {
          uint32_t glyphID = skin()->mapGlyph(GlyphKey(text[i], fontSize, false, yg::Color(0, 0, 0, 0)), fixedFont);
          CharStyle const * p = static_cast<CharStyle const *>(skin()->fromID(glyphID));
          rect.Add(pt);
          rect.Add(pt + m2::PointD(p->m_xOffset + p->m_texRect.SizeX() - 4, -p->m_yOffset - (int)p->m_texRect.SizeY() + 4));
          pt += m2::PointD(p->m_xAdvance, 0);
        }
        else
        {
          GlyphMetrics const m = resourceManager()->getGlyphMetrics(GlyphKey(text[i], fontSize, false, yg::Color(0, 0, 0, 0)));

          rect.Add(pt);
          rect.Add(pt + m2::PointD(m.m_xOffset + m.m_width, - m.m_yOffset - m.m_height));
          pt += m2::PointD(m.m_xAdvance, 0);
        }
      }

      rect.Inflate(2, 2);

      return rect;
    }

    /// Encapsulate array of points in readable getting direction.
    class pts_array
    {
      m2::PointD const * m_arr;
      size_t m_size;
      bool m_reverse;

    public:
      pts_array(m2::PointD const * arr, size_t sz, double fullLength, double & pathOffset)
        : m_arr(arr), m_size(sz), m_reverse(false)
      {
        ASSERT ( m_size > 1, () );

        /* assume, that readable text in path should be ('o' - start draw point):
         *    /   o
         *   /     \
         *  /   or  \
         * o         \
        */

        double const a = ang::AngleTo(m_arr[0], m_arr[m_size-1]);
        if (fabs(a) > math::pi / 2.0)
        {
          // if we swap direction, we need to recalculate path offset from the end
          double len = 0.0;
          for (size_t i = 1; i < m_size; ++i)
            len += m_arr[i-1].Length(m_arr[i]);

          pathOffset = fullLength - pathOffset - len;
          ASSERT ( pathOffset >= -1.0E-6, () );
          if (pathOffset < 0.0) pathOffset = 0.0;

          m_reverse = true;
        }
      }

      size_t size() const { return m_size; }

      m2::PointD get(size_t i) const
      {
        ASSERT ( i < m_size, ("Index out of range") );
        return m_arr[m_reverse ? m_size - i - 1 : i];
      }
      m2::PointD operator[](size_t i) const { return get(i); }
    };

    double const angle_not_inited = -100.0;

    bool CalcPointAndAngle(pts_array const & arr, double offset, size_t & ind, m2::PointD & pt, double & angle)
    {
      size_t const oldInd = ind;

      while (true)
      {
        if (ind + 1 == arr.size())
          return false;

        double const l = arr[ind + 1].Length(pt);
        if (offset < l)
          break;

        offset -= l;
        pt = arr[++ind];
      }

      if (oldInd != ind || angle == angle_not_inited)
        angle = ang::AngleTo(pt, arr[ind + 1]);
      pt = pt.Move(offset, angle);
      return true;
    }

    bool TextRenderer::drawPathText(
        m2::PointD const * path, size_t s, uint8_t fontSize, yg::Color const & color, string const & utf8Text,
        double fullLength, double pathOffset, TextPos pos, bool isMasked, yg::Color const & maskColor, double depth, bool isFixedFont)
    {
      if (m_useTextTree)
        checkTextRedraw();

      if (isMasked)
        if (!drawPathTextImpl(path, s, fontSize, maskColor, utf8Text, fullLength, pathOffset, pos, true, depth, isFixedFont))
          return false;
      return drawPathTextImpl(path, s, fontSize, color, utf8Text, fullLength, pathOffset, pos, false, depth, isFixedFont);
    }

    bool TextRenderer::drawPathTextImpl(
        m2::PointD const * path, size_t s, uint8_t fontSize, yg::Color const & color, string const & utf8Text,
        double fullLength, double pathOffset, TextPos pos, bool isMasked, double depth, bool isFixedFont)
    {
      pts_array arrPath(path, s, fullLength, pathOffset);

      wstring const text = Log2Vis(FromUtf8(utf8Text));

      // calculate base line offset
      float blOffset = 2;
      switch (pos)
      {
      case under_line: blOffset -= fontSize; break;
      case middle_line: blOffset -= fontSize / 2; break;
      case above_line: blOffset -= 0; break;
      }

      size_t const count = text.size();

      vector<GlyphMetrics> glyphs(count);

      // get vector of glyphs and calculate string length
      double strLength = 0.0;
      for (size_t i = 0; i < count; ++i)
      {
        glyphs[i] = resourceManager()->getGlyphMetrics(GlyphKey(text[i], fontSize, isMasked, yg::Color(0, 0, 0, 0)));
        strLength += glyphs[i].m_xAdvance;
      }

      // offset of the text from path's start
      double offset = (fullLength - strLength) / 2.0;
      if (offset < 0.0) return false;
      offset -= pathOffset;
      if (-offset >= strLength) return false;

      // find first visible glyph
      size_t i = 0;
      while (offset < 0 && i < count)
        offset += glyphs[i++].m_xAdvance;

      size_t ind = 0;
      m2::PointD ptOrg = arrPath[0];
      double angle = angle_not_inited;

      // draw visible glyphs
      for (; i < count; ++i)
      {
        if (!CalcPointAndAngle(arrPath, offset, ind, ptOrg, angle))
          break;

        uint32_t const glyphID = skin()->mapGlyph(GlyphKey(text[i], fontSize, isMasked, color), isFixedFont);
        CharStyle const * charStyle = static_cast<CharStyle const *>(skin()->fromID(glyphID));

        drawGlyph(ptOrg, m2::PointD(0.0, 0.0), angle, blOffset, charStyle, depth);

        offset = glyphs[i].m_xAdvance;
      }

      return true;
    }

    void TextRenderer::drawGlyph(m2::PointD const & ptOrg, m2::PointD const & ptGlyph, float angle, float blOffset, CharStyle const * p, double depth)
    {
      float x0 = ptGlyph.x + (p->m_xOffset - 1);
      float y1 = ptGlyph.y - (p->m_yOffset - 1) - blOffset;
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

    void TextRenderer::drawPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
    {
      if (m_useTextTree)
        checkTextRedraw();
      base_t::drawPath(points, pointsCount, styleID, depth);
    }
  }
}
