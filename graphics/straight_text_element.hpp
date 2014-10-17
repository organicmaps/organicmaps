#pragma once

#include "text_element.hpp"
#include "glyph_layout.hpp"


namespace graphics
{
  class StraightTextElement : public TextElement
  {
  private:

    // Glyph layout of the text parts.
    // In 99% cases cantainers will hold 1 element.
    buffer_vector<GlyphLayout, 1> m_glyphLayouts;
    buffer_vector<m2::PointD, 1> m_offsets;

  public:

    struct Params : TextElement::Params
    {
      unsigned m_minWordsInRow;
      unsigned m_maxWordsInRow;
      unsigned m_minSymInRow;
      unsigned m_maxSymInRow;
      unsigned m_maxPixelWidth;
      bool m_doSplit;
      bool m_doForceSplit;
      bool m_useAllParts;
      m2::PointD m_offset;
      string m_delimiters;
      Params();
    };

    StraightTextElement(Params const & p);

    virtual void GetMiniBoundRects(RectsT & rects) const;

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void setPivot(m2::PointD const & pv, bool dirtyFlag = true);

    void setTransformation(const math::Matrix<double, 3, 3> & m);

    bool hasSharpGeometry() const;
  };
}
