#pragma once

#include "text_element.hpp"
#include "glyph_layout.hpp"


namespace graphics
{
  class PathTextElement : public TextElement
  {
  private:

    GlyphLayout m_glyphLayout;

  public:

    struct Params : TextElement::Params
    {
      m2::PointD const * m_pts;
      size_t m_ptsCount;
      double m_fullLength;
      double m_pathOffset;
      double m_textOffset;
      Params();
    };

    PathTextElement(Params const & p);

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void setPivot(m2::PointD const & pivot);
    void setTransformation(const math::Matrix<double, 3, 3> & m);
  };
}
