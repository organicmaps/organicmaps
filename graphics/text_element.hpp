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

    /// text-element specific
    FontDesc m_fontDesc;
    FontDesc m_auxFontDesc;

    strings::UniString m_logText; //< logical string
    strings::UniString m_auxLogText;

    strings::UniString m_visText; //< visual string, BIDI processed
    strings::UniString m_auxVisText;

    bool m_log2vis;

    mutable vector<m2::AnyRectD> m_boundRects;

    bool isBidi() const;
    bool isAuxBidi() const;

  public:

    struct Params : OverlayElement::Params
    {
      FontDesc m_fontDesc;
      strings::UniString m_logText;

      FontDesc m_auxFontDesc;
      strings::UniString m_auxLogText;

      bool m_log2vis;
      GlyphCache * m_glyphCache;
    };

    TextElement(Params const & p);

    void drawTextImpl(GlyphLayout const & layout,
                      OverlayRenderer * r,
                      math::Matrix<double, 3, 3> const & m,
                      bool doTransformPivotOnly,
                      bool doAlignPivot,
                      FontDesc const & desc,
                      double depth) const;
    strings::UniString const & logText() const;
    strings::UniString const & auxLogText() const;
    strings::UniString const & visText() const;
    strings::UniString const & auxVisText() const;
    FontDesc const & fontDesc() const;
    FontDesc const & auxFontDesc() const;
  };
}
