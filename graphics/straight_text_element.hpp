#pragma once

#include "text_element.hpp"

namespace graphics
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
      bool m_useAllParts;
      m2::PointD m_offset;
      string m_delimiters;
      Params();
    };

    StraightTextElement(Params const & p);

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void setPivot(m2::PointD const & pv);

    void setTransformation(const math::Matrix<double, 3, 3> & m);

    bool hasSharpGeometry() const;
  };
}
