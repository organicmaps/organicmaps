#pragma once

#include "../base/string_utils.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/aa_rect2d.hpp"

#include "../std/shared_ptr.hpp"

#include "defines.hpp"
#include "font_desc.hpp"
#include "glyph_layout.hpp"

namespace yg
{
  class ResourceManager;
  class Skin;

  namespace gl
  {
    class Screen;
    class TextRenderer;
  }

  class OverlayElement
  {
  private:

    m2::PointD m_pivot;
    yg::EPosition m_position;
    double m_depth;

  public:

    struct Params
    {
      m2::PointD m_pivot;
      yg::EPosition m_position;
      double m_depth;
    };

    OverlayElement(Params const & p);

    virtual void offset(m2::PointD const & offs) = 0;
    virtual m2::AARectD const boundRect() const = 0;
    virtual void draw(gl::Screen * screen) const = 0;

    m2::PointD const & pivot() const;
    void setPivot(m2::PointD const & pv);

    yg::EPosition position() const;
    void setPosition(yg::EPosition pos);

    double depth() const;
    void setDepth(double depth);
  };

  class TextElement : public OverlayElement
  {
  protected:

    /// text-element specific
    FontDesc m_fontDesc;
    strings::UniString m_logText; //< logical string
    strings::UniString m_visText; //< visual string, BIDI processed
    bool m_log2vis;
    ResourceManager * m_rm;
    Skin * m_skin;

    strings::UniString log2vis(strings::UniString const & str);

  public:

    struct Params : OverlayElement::Params
    {
      FontDesc m_fontDesc;
      strings::UniString m_logText;
      bool m_log2vis;
      ResourceManager * m_rm;
      Skin * m_skin;
    };

    TextElement(Params const & p);

    void drawTextImpl(GlyphLayout const & layout, gl::TextRenderer * screen, FontDesc const & desc, double depth) const;
    strings::UniString const & logText() const;
    strings::UniString const & visText() const;
    FontDesc const & fontDesc() const;
  };

  class StraightTextElement : public TextElement
  {
  private:

    GlyphLayout m_glyphLayout;

  public:

    struct Params : TextElement::Params
    {};

    StraightTextElement(Params const & p);

    m2::AARectD const boundRect() const;
    void draw(gl::Screen * screen) const;
    void draw(gl::TextRenderer * screen) const;
    void offset(m2::PointD const & offs);
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

    m2::AARectD const boundRect() const;
    void draw(gl::Screen * screen) const;
    void draw(gl::TextRenderer * screen) const;
    void offset(m2::PointD const & offs);
  };
}
