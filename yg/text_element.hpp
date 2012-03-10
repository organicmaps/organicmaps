#pragma once

#include "../base/string_utils.hpp"

#include "../base/matrix.hpp"

#include "../geometry/any_rect2d.hpp"

#include "../std/shared_ptr.hpp"

#include "font_desc.hpp"
#include "glyph_layout.hpp"
#include "overlay_element.hpp"

namespace yg
{
  class ResourceManager;
  class Skin;
  class SkinPage;

  namespace gl
  {
    class OverlayRenderer;
  }

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

  protected:

    using OverlayElement::map;
    using OverlayElement::find;
    using OverlayElement::getNonPackedRects;

    void map(GlyphLayout const & layout,
             ResourceStyleCache * stylesCache,
             FontDesc const & desc) const;

    bool find(GlyphLayout const & layout,
              ResourceStyleCache * stylesCache,
              FontDesc const & desc) const;

    void getNonPackedRects(GlyphLayout const & layout,
                           FontDesc const & desc,
                           ResourceStyleCache * stylesCache,
                           vector<m2::PointU> & v) const;

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
                      gl::OverlayRenderer * r,
                      math::Matrix<double, 3, 3> const & m,
                      bool doTransformPivotOnly,
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
