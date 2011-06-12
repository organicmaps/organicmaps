#pragma once

#include "shape_renderer.hpp"
#include "defines.hpp"
#include "font_desc.hpp"
#include "text_element.hpp"

#include "../geometry/tree4d.hpp"

#include "../std/shared_ptr.hpp"


namespace yg
{
  namespace gl
  {
    class TextRenderer : public ShapeRenderer
    {
    private:

      bool drawPathTextImpl(FontDesc const & fontDesc,
                            m2::PointD const * path,
                            size_t s,
                            string const & utf8Text,
                            double fullLength,
                            double pathOffset,
                            yg::EPosition pos,
                            double depth);

      /// Drawing text from point rotated by the angle.
      void drawTextImpl(FontDesc const & fontDesc,
                        m2::PointD const & pt,
                        yg::EPosition pos,
                        float angle,
                        string const & utf8Text,
                        double depth,
                        bool log2vis);

      bool m_drawTexts;
      int m_glyphCacheID;

    public:

      typedef ShapeRenderer base_t;

      struct Params : base_t::Params
      {
        bool m_drawTexts;
        int m_glyphCacheID;
        Params();
      };

      TextRenderer(Params const & params);

      void drawGlyph(m2::PointD const & ptOrg,
                     m2::PointD const & ptGlyph,
                     ang::AngleD const & angle,
                     float blOffset,
                     CharStyle const * p,
                     double depth);

      /// Drawing text from point rotated by the angle.
      void drawText(FontDesc const & fontDesc,
                    m2::PointD const & pt,
                    yg::EPosition pos,
                    float angle,
                    string const & utf8Text,
                    double depth,
                    bool log2vis);

      /// Drawing text in the middle of the path.
      bool drawPathText(FontDesc const & fontDesc,
                        m2::PointD const * path,
                        size_t s,
                        string const & utf8Text,
                        double fullLength,
                        double pathOffset,
                        yg::EPosition pos,
                        double depth);

      GlyphCache * glyphCache() const;
    };
  }
}
