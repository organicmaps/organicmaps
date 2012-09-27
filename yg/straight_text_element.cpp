#include "../base/SRC_FIRST.hpp"
#include "straight_text_element.hpp"
#include "overlay_renderer.hpp"

namespace yg
{
  void visSplit(strings::UniString const & visText,
                buffer_vector<strings::UniString, 3> & res,
                char const * delimiters,
                size_t delimSize,
                bool splitAllFound)
  {
    if (!splitAllFound)
    {
      if (visText.size() > 15)
      {
        /// split into two
        size_t rs = visText.size() / 2;
        size_t ls = visText.size() / 2;

        size_t s;

        while (true)
        {
          if (rs == visText.size() - 1)
            break;

          bool foundDelim = false;

          for (int i = 0; i < delimSize; ++i)
            if (visText[rs] == strings::UniChar(delimiters[i]))
            {
              foundDelim = true;
              break;
            }

          if (foundDelim)
            break;

          ++rs;
        }

        if (rs == visText.size() - 1)
        {
          while (true)
          {
            if (ls == 0)
              break;

            bool foundDelim = false;

            for (int i = 0; i < delimSize; ++i)
              if (visText[ls] == strings::UniChar(delimiters[i]))
              {
                foundDelim = true;
                break;
              }

            if (foundDelim)
              break;

            --ls;
          }

          if (ls < 5)
            s = visText.size() - 1;
          else
            s = ls;
        }
        else
          s = rs;

        res.push_back(strings::UniString());
        res.back().resize(s + 1);
        for (unsigned i = 0; i < s + 1; ++i)
          res.back()[i] = visText[i];

        if (s != visText.size() - 1)
        {
          res.push_back(strings::UniString());
          res.back().resize(visText.size() - s - 1);
          for (unsigned i = s + 1; i < visText.size(); ++i)
            res.back()[i - s - 1] = visText[i];
        }
      }
      else
        res.push_back(visText);
    }
    else
    {
      size_t beg = 0;
      size_t i = 0;
      for (;i < visText.size(); ++i)
      {
        for (int j = 0; j < delimSize; ++j)
          if (visText[i] == strings::UniChar(delimiters[j]))
          {
            strings::UniString s;
            s.resize(i - beg);
            for (unsigned k = beg; k < i; ++k)
              s[k - beg] = visText[k];
            res.push_back(s);
            beg = i + 1;
          }
      }

      strings::UniString s;

      s.resize(i - beg);
      for (unsigned k = beg; k < i; ++k)
        s[k - beg] = visText[k];

      res.push_back(s);
    }
  }

  StraightTextElement::StraightTextElement(Params const & p)
    : TextElement(p)
  {
    ASSERT(p.m_fontDesc.IsValid(), ());

    buffer_vector<strings::UniString, 3> res;
    if (p.m_doSplit && !isBidi())
    {
      res.clear();
      if (!p.m_delimiters.empty())
        visSplit(visText(), res, p.m_delimiters.c_str(), p.m_delimiters.size(), p.m_useAllParts);
      else
        visSplit(visText(), res, " \n\t", 3, p.m_useAllParts);
    }
    else
      res.push_back(visText());

    double allElemWidth = 0;
    double allElemHeight = 0;

    for (unsigned i = 0; i < res.size(); ++i)
    {
      m_glyphLayouts.push_back(GlyphLayout(p.m_glyphCache, p.m_fontDesc, m2::PointD(0, 0), res[i], yg::EPosCenter));
      m2::RectD r = m_glyphLayouts.back().boundRects().back().GetGlobalRect();
      allElemWidth = max(r.SizeX(), allElemWidth);
      allElemHeight += r.SizeY();
    }

    buffer_vector<strings::UniString, 3> auxRes;

    if (p.m_auxFontDesc.IsValid() && (!auxVisText().empty()))
    {
      GlyphLayout l(p.m_glyphCache, p.m_auxFontDesc, m2::PointD(0, 0), auxVisText(), yg::EPosCenter);
      if (l.boundRects().back().GetGlobalRect().SizeX() > allElemWidth)
      {
        // should split
        if (p.m_doSplit && !isAuxBidi())
        {
          if (!p.m_delimiters.empty())
            visSplit(auxVisText(), auxRes, p.m_delimiters.c_str(), p.m_delimiters.size(), p.m_useAllParts);
          else
            visSplit(auxVisText(), auxRes, " \n\t", 3, p.m_useAllParts);
        }
        else
          auxRes.push_back(auxVisText());
      }
      else
        auxRes.push_back(auxVisText());

      for (int i = 0; i < auxRes.size(); ++i)
      {
        m_glyphLayouts.push_back(GlyphLayout(p.m_glyphCache, p.m_auxFontDesc, m2::PointD(0, 0), auxRes[i], yg::EPosCenter));
        m2::RectD r = m_glyphLayouts.back().boundRects().back().GetGlobalRect();
        allElemWidth = max(r.SizeX(), allElemWidth);
        allElemHeight += r.SizeY();
      }
    }


    double curShift = allElemHeight / 2;

    /// performing aligning of glyphLayouts as for the center position

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      double elemSize = m_glyphLayouts[i].boundRects().back().GetGlobalRect().SizeY();
      m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(0, -curShift + elemSize / 2));
      curShift -= elemSize;
    }

    if (position() & yg::EPosLeft)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(-allElemWidth / 2, 0));

    if (position() & yg::EPosRight)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(allElemWidth / 2, 0));

    if (position() & yg::EPosAbove)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(0, -allElemHeight / 2));

    if (position() & yg::EPosUnder)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(0, allElemHeight / 2));

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      m_offsets.push_back(m_glyphLayouts[i].pivot());
      m_glyphLayouts[i].setPivot(m_offsets[i] + pivot());
    }
  }

  StraightTextElement::Params::Params()
    : m_minWordsInRow(2),
      m_maxWordsInRow(4),
      m_minSymInRow(10),
      m_maxSymInRow(20),
      m_doSplit(false)
  {}

  StraightTextElement::StraightTextElement(StraightTextElement const & src,
                                           math::Matrix<double, 3, 3> const & m)
    : TextElement(src),
      m_glyphLayouts(src.m_glyphLayouts)
  {
    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
      m_offsets = src.m_offsets;

    setPivot(pivot() * m);

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      m_glyphLayouts[i].setPivot(pivot());
      m_glyphLayouts[i].setOffset(m_offsets[i]);
    }
  }

  vector<m2::AnyRectD> const & StraightTextElement::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();

      for (size_t i = 0; i < m_glyphLayouts.size(); ++i)
        copy(m_glyphLayouts[i].boundRects().begin(),
             m_glyphLayouts[i].boundRects().end(),
             back_inserter(m_boundRects));

      setIsDirtyRect(false);
    }
    return m_boundRects;
  }

  void StraightTextElement::draw(gl::OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m) const
  {
    if (screen->isDebugging())
    {
      yg::Color c(255, 255, 255, 32);

      if (isFrozen())
        c = yg::Color(0, 0, 255, 64);
      if (isNeedRedraw())
        c = yg::Color(255, 0, 0, 64);

      screen->drawRectangle(roughBoundRect(), yg::Color(255, 255, 0, 64), yg::maxDepth - 3);

      for (unsigned i = 0 ; i < boundRects().size(); ++i)
        screen->drawRectangle(boundRects()[i], c, yg::maxDepth - 3);
    }

    if (!isNeedRedraw())
      return;

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      if (m_glyphLayouts[i].fontDesc().m_isMasked)
        drawTextImpl(m_glyphLayouts[i], screen, m, true, true, m_glyphLayouts[i].fontDesc(), yg::maxDepth - 1);
    }

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      yg::FontDesc fontDesc = m_glyphLayouts[i].fontDesc();
      fontDesc.m_isMasked = false;
      drawTextImpl(m_glyphLayouts[i], screen, m, true, true, fontDesc, yg::maxDepth);
    }
  }

  void StraightTextElement::setPivot(m2::PointD const & pv)
  {
    m2::PointD oldPv = pivot();
    m2::PointD offs = pv - oldPv;

    TextElement::setPivot(pv);

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
      m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + offs);
  }

  int StraightTextElement::visualRank() const
  {
    return 1000 + m_fontDesc.m_size;
  }

  OverlayElement * StraightTextElement::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new StraightTextElement(*this, m);
  }

  bool StraightTextElement::hasSharpGeometry() const
  {
    return true;
  }
}
