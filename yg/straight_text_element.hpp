#pragma once

#include "text_element.hpp"

namespace yg
{
  class StraightTextElement : public TextElement
  {
  private:

    /// glyph layout of the text parts.
    vector<GlyphLayout> m_glyphLayouts;
    vector<m2::PointD> m_offsets;

  public:

    struct Params : TextElement::Params
    {
      unsigned m_minWordsInRow;
      unsigned m_maxWordsInRow;
      unsigned m_minSymInRow;
      unsigned m_maxSymInRow;
      bool m_doSplit;
      string m_delimiters;
      Params();
    };

    StraightTextElement(Params const & p);
    StraightTextElement(StraightTextElement const & src, math::Matrix<double, 3, 3> const & m);

    vector<m2::AARectD> const & boundRects() const;

    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void fillUnpacked(StylesCache * stylesCache, vector<m2::PointU> & v) const;
    bool find(StylesCache * stylesCache) const;
    void map(StylesCache * stylesCache) const;

    int visualRank() const;

    void offset(m2::PointD const & offs);

    OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
  };
}
