#pragma once
#include "overlay_element.hpp"
#include "font_desc.hpp"

#include "../base/matrix.hpp"
#include "../base/string_utils.hpp"


namespace graphics
{
  class GlyphLayout;
  class GlyphCache;
  class OverlayRenderer;

  class TextElement : public OverlayElement
  {
  protected:

    FontDesc m_fontDesc, m_auxFontDesc;

    mutable vector<m2::AnyRectD> m_boundRects;

  public:

    struct Params : OverlayElement::Params
    {
      FontDesc m_fontDesc;
      strings::UniString m_logText;

      FontDesc m_auxFontDesc;
      strings::UniString m_auxLogText;

      bool m_log2vis;
      GlyphCache * m_glyphCache;

      pair<bool, bool> GetVisibleTexts(strings::UniString & visText,
                                       strings::UniString & auxVisText) const;
    };

    TextElement(Params const & p);

    void drawTextImpl(GlyphLayout const & layout,
                      OverlayRenderer * r,
                      math::Matrix<double, 3, 3> const & m,
                      bool doTransformPivotOnly,
                      bool doAlignPivot,
                      FontDesc const & desc,
                      double depth) const;

    FontDesc const & fontDesc() const;
    FontDesc const & auxFontDesc() const;
  };
}
