#include "../base/SRC_FIRST.hpp"

#include "straight_text_element.hpp"
#include "overlay_renderer.hpp"

#include "../std/iterator.hpp"
#include "../std/algorithm.hpp"


namespace graphics
{
  void visSplit(strings::UniString const & visText,
                buffer_vector<strings::UniString, 3> & res,
                char const * delims,
                bool splitAllFound)
  {
    if (!splitAllFound)
    {
      size_t count = visText.size();
      if (count > 15)
      {
        // split on two parts
        typedef strings::UniString::const_iterator IterT;
        IterT const iMiddle = visText.begin() + count/2;

        size_t const delimsSize = strlen(delims);

        // find next delimeter after middle [m, e)
        IterT iNext = find_first_of(iMiddle,
                                    visText.end(),
                                    delims, delims + delimsSize);

        // find last delimeter before middle [b, m)
        IterT iPrev = find_first_of(reverse_iterator<IterT>(iMiddle),
                                    reverse_iterator<IterT>(visText.begin()),
                                    delims, delims + delimsSize).base();
        // don't do split like this:
        //     xxxx
        // xxxxxxxxxxxx
        if (4 * distance(visText.begin(), iPrev) <= count)
          iPrev = visText.end();
        else
          --iPrev;

        // get closest delimiter to the middle
        if (iNext == visText.end() ||
            (iPrev != visText.end() && distance(iPrev, iMiddle) < distance(iMiddle, iNext)))
        {
          iNext = iPrev;
        }

        // split string on 2 parts
        if (iNext != visText.end())
        {
          ASSERT_NOT_EQUAL(iNext, visText.begin(), ());
          res.push_back(strings::UniString(visText.begin(), iNext));

          if (++iNext != visText.end())
            res.push_back(strings::UniString(iNext, visText.end()));

          return;
        }
      }

      res.push_back(visText);
    }
    else
    {
      // split string using according to all delimiters
      typedef strings::SimpleDelimiter DelimT;
      for (strings::TokenizeIterator<DelimT> iter(visText, DelimT(delims)); iter; ++iter)
        res.push_back(iter.GetUniString());
    }
  }

  StraightTextElement::StraightTextElement(Params const & p)
    : TextElement(p)
  {
    ASSERT(p.m_fontDesc.IsValid(), ());

    double allElemWidth = 0;
    double allElemHeight = 0;

    if (!visText().empty())
    {
      buffer_vector<strings::UniString, 3> res;
      if (p.m_doSplit && !isBidi())
      {
        res.clear();
        if (!p.m_delimiters.empty())
          visSplit(visText(), res, p.m_delimiters.c_str(), p.m_useAllParts);
        else
          visSplit(visText(), res, " \n\t", p.m_useAllParts);
      }
      else
        res.push_back(visText());

      for (unsigned i = 0; i < res.size(); ++i)
      {
        m_glyphLayouts.push_back(GlyphLayout(p.m_glyphCache, p.m_fontDesc, m2::PointD(0, 0), res[i], graphics::EPosCenter));
        m2::RectD r = m_glyphLayouts.back().boundRects().back().GetGlobalRect();
        allElemWidth = max(r.SizeX(), allElemWidth);
        allElemHeight += r.SizeY();
      }
    }

    if (p.m_auxFontDesc.IsValid() && !auxVisText().empty())
    {
      buffer_vector<strings::UniString, 3> auxRes;

      GlyphLayout l(p.m_glyphCache, p.m_auxFontDesc, m2::PointD(0, 0), auxVisText(), graphics::EPosCenter);
      if (l.boundRects().back().GetGlobalRect().SizeX() > allElemWidth)
      {
        // should split
        if (p.m_doSplit && !isAuxBidi())
        {
          if (!p.m_delimiters.empty())
            visSplit(auxVisText(), auxRes, p.m_delimiters.c_str(), p.m_useAllParts);
          else
            visSplit(auxVisText(), auxRes, " \n\t", p.m_useAllParts);
        }
        else
          auxRes.push_back(auxVisText());
      }
      else
        auxRes.push_back(auxVisText());

      for (int i = 0; i < auxRes.size(); ++i)
        m_glyphLayouts.push_back(GlyphLayout(p.m_glyphCache, p.m_auxFontDesc, m2::PointD(0, 0), auxRes[i], graphics::EPosCenter));
    }

    double curShift = allElemHeight / 2;

    /// performing aligning of glyphLayouts as for the center position

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      double elemSize = m_glyphLayouts[i].boundRects().back().GetGlobalRect().SizeY();
      m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(0, -curShift + elemSize / 2) + p.m_offset);
      curShift -= elemSize;
    }

    if (position() & graphics::EPosLeft)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(-allElemWidth / 2, 0));

    if (position() & graphics::EPosRight)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(allElemWidth / 2, 0));

    if (position() & graphics::EPosAbove)
      for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
        m_glyphLayouts[i].setPivot(m_glyphLayouts[i].pivot() + m2::PointD(0, -allElemHeight / 2));

    if (position() & graphics::EPosUnder)
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
      m_doSplit(false),
      m_useAllParts(true),
      m_offset(0, 0)
  {}

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

  void StraightTextElement::draw(OverlayRenderer * screen, math::Matrix<double, 3, 3> const & m) const
  {
    int doffs = 0;
    if (screen->isDebugging())
    {
      graphics::Color c(255, 255, 255, 32);

      if (isFrozen())
        c = graphics::Color(0, 0, 255, 64);
      if (isNeedRedraw())
        c = graphics::Color(255, 0, 0, 64);

      screen->drawRectangle(roughBoundRect(), graphics::Color(255, 255, 0, 64), depth());

      doffs += 1;

      for (unsigned i = 0 ; i < boundRects().size(); ++i)
        screen->drawRectangle(boundRects()[i], c, depth() + doffs);

      doffs += 1;
    }

    if (!isNeedRedraw())
      return;

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      if (m_glyphLayouts[i].fontDesc().m_isMasked)
        drawTextImpl(m_glyphLayouts[i], screen, m, true, true, m_glyphLayouts[i].fontDesc(), depth() + doffs);
    }

    doffs += 1;

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      graphics::FontDesc fontDesc = m_glyphLayouts[i].fontDesc();
      fontDesc.m_isMasked = false;
      drawTextImpl(m_glyphLayouts[i], screen, m, true, true, fontDesc, depth() + doffs);
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

  void StraightTextElement::setTransformation(const math::Matrix<double, 3, 3> & m)
  {
    setPivot(pivot() * getResetMatrix() * m);

    for (unsigned i = 0; i < m_glyphLayouts.size(); ++i)
    {
      m_glyphLayouts[i].setPivot(pivot());
      m_glyphLayouts[i].setOffset(m_offsets[i]);
    }

    TextElement::setTransformation(m);
  }

  bool StraightTextElement::hasSharpGeometry() const
  {
    return true;
  }
}
