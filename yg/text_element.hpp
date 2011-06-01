#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

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
  }

  class OverlayElement
  {
  private:

    m2::PointD m_pivot;
    yg::EPosition m_position;

  public:

    struct Params
    {
      m2::PointD m_pivot;
      yg::EPosition m_position;
    };

    OverlayElement(Params const & p);

    virtual void offset(m2::PointD const & offs) = 0;
    virtual m2::RectD const boundRect() const = 0;
    virtual void draw(gl::Screen * screen) = 0;

    m2::PointD const & pivot() const;
    void setPivot(m2::PointD const & pv);

    yg::EPosition position() const;
    void setPosition(yg::EPosition pos);
  };

  class TextElement : public OverlayElement
  {
  protected:

    /// text-element specific
    FontDesc m_fontDesc;
    string m_utf8Text;
    double m_depth;
    bool m_log2vis;
    shared_ptr<ResourceManager> m_rm;
    shared_ptr<Skin> m_skin;

  public:

    struct Params : OverlayElement::Params
    {
      FontDesc m_fontDesc;
      string m_utf8Text;
      double m_depth;
      bool m_log2vis;
      shared_ptr<ResourceManager> m_rm;
      shared_ptr<Skin> m_skin;
    };

    TextElement(Params const & p);

    void drawTextImpl(GlyphLayout const & layout, gl::Screen * screen, FontDesc const & desc);
  };

  class StraightTextElement : public TextElement
  {
  private:

    GlyphLayout m_glyphLayout;

  public:

    struct Params : TextElement::Params
    {};

    StraightTextElement(Params const & p);

    m2::RectD const boundRect() const;
    void draw(gl::Screen * screen);
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

    m2::RectD const boundRect() const;
    void draw(gl::Screen * screen);
    void offset(m2::PointD const & offs);
  };
}
