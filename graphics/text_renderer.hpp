#pragma once

#include "image_renderer.hpp"
#include "defines.hpp"
#include "font_desc.hpp"
#include "text_element.hpp"

#include "../geometry/tree4d.hpp"

#include "../std/shared_ptr.hpp"


namespace graphics
{
  namespace gl
  {
    class TextRenderer : public ImageRenderer
    {
    private:

      bool m_drawTexts;
      int m_glyphCacheID;

    public:

      typedef ImageRenderer base_t;

      struct Params : base_t::Params
      {
        bool m_drawTexts;
        int m_glyphCacheID;
        Params();
      };

      TextRenderer(Params const & params);

      void drawStraightGlyph(m2::PointD const & ptOrg,
                             m2::PointD const & ptGlyph,
                             GlyphStyle const * p,
                             float depth);

      void drawGlyph(m2::PointD const & ptOrg,
                     m2::PointD const & ptGlyph,
                     ang::AngleD const & angle,
                     float blOffset,
                     GlyphStyle const * p,
                     double depth);


      GlyphCache * glyphCache() const;
    };
  }
}
