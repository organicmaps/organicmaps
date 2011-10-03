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
    strings::UniString m_logText; //< logical string
    strings::UniString m_visText; //< visual string, BIDI processed
    bool m_log2vis;
    GlyphCache * m_glyphCache;

    mutable vector<m2::AnyRectD> m_boundRects;

    bool isBidi() const;

  protected:

    void map(GlyphLayout const & layout,
             StylesCache * stylesCache,
             FontDesc const & desc) const;

    bool find(GlyphLayout const & layout,
              StylesCache * stylesCache,
              FontDesc const & desc) const;

    void fillUnpacked(GlyphLayout const & layout,
                      FontDesc const & desc,
                      StylesCache * stylesCache,
                      vector<m2::PointU> & v) const;

  public:

    struct Params : OverlayElement::Params
    {
      FontDesc m_fontDesc;
      strings::UniString m_logText;
      bool m_log2vis;
      GlyphCache * m_glyphCache;
    };

    TextElement(Params const & p);

    void drawTextImpl(GlyphLayout const & layout,
                      gl::OverlayRenderer * r,
                      math::Matrix<double, 3, 3> const & m,
                      FontDesc const & desc,
                      double depth) const;
    strings::UniString const & logText() const;
    strings::UniString const & visText() const;
    FontDesc const & fontDesc() const;
  };
}
