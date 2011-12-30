#pragma once

#include "text_element.hpp"

namespace yg
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
    };

    PathTextElement(Params const & p);
    PathTextElement(PathTextElement const & src, math::Matrix<double, 3, 3> const & m);

    vector<m2::AnyRectD> const & boundRects() const;

    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void getNonPackedRects(StylesCache * stylesCache, vector<m2::PointU> & v) const;
    bool find(StylesCache * stylesCache) const;
    void map(StylesCache * stylesCache) const;

    int visualRank() const;

    void offset(m2::PointD const & offs);

    OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
  };
}
