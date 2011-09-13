#pragma once

#include "../base/string_utils.hpp"

#include "../base/matrix.hpp"

#include "../geometry/aa_rect2d.hpp"

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

  public:

    struct Params : OverlayElement::Params
    {
      FontDesc m_fontDesc;
      strings::UniString m_logText;
      bool m_log2vis;
      GlyphCache * m_glyphCache;
    };

    TextElement(Params const & p);

    void cacheTextImpl(GlyphLayout const & layout,
                       vector<shared_ptr<SkinPage> > & skinPages,
                       shared_ptr<ResourceManager> const & rm,
                       GlyphCache * glyphCache,
                       FontDesc const & desc) const;

    void drawTextImpl(GlyphLayout const & layout,
                      gl::OverlayRenderer * r,
                      math::Matrix<double, 3, 3> const & m,
                      FontDesc const & desc,
                      double depth) const;
    strings::UniString const & logText() const;
    strings::UniString const & visText() const;
    FontDesc const & fontDesc() const;
  };

  class StraightTextElement : public TextElement
  {
  private:

    /// glyph layout aligned to pivot point
    GlyphLayout m_glyphLayout;

  public:

    struct Params : TextElement::Params
    {};

    StraightTextElement(Params const & p);
    StraightTextElement(StraightTextElement const & src, math::Matrix<double, 3, 3> const & m);

    m2::AARectD const boundRect() const;
    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    void offset(m2::PointD const & offs);
    void cache(vector<shared_ptr<SkinPage> > & skinPages,
               shared_ptr<ResourceManager> const & rm,
               GlyphCache * glyphCache) const;
  };

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

    m2::AARectD const boundRect() const;
    void draw(gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    void offset(m2::PointD const & offs);

    void cache(vector<shared_ptr<SkinPage> > & skinPages,
               shared_ptr<ResourceManager> const & rm,
               GlyphCache * glyphCache) const;
  };
}
