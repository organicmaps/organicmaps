#pragma once

#include "text_element.hpp"
#include "glyph_layout.hpp"


namespace graphics
{
  class PathTextElement : public TextElement
  {
    typedef TextElement BaseT;

    GlyphLayoutPath m_glyphLayout;

    /// Cached bound rect for the fast Overlay tree routine.
    mutable m2::RectD m_boundRect;

  public:
    struct Params : BaseT::Params
    {
      m2::PointD const * m_pts;
      size_t m_ptsCount;
      double m_fullLength;
      double m_pathOffset;
      double m_textOffset;
      Params();
    };

    PathTextElement(Params const & p);

    virtual m2::RectD GetBoundRect() const;
    virtual void GetMiniBoundRects(RectsT & rects) const;

    void draw(OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void setPivot(m2::PointD const & pivot, bool dirtyFlag = true);
    void setTransformation(const math::Matrix<double, 3, 3> & m);
  };
}
