#pragma once

#include "element.hpp"

#include "../base/string_utils.hpp"
#include "../base/matrix.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/unique_ptr.hpp"


namespace graphics
{
  class DisplayList;
  class GlyphLayout;
}

namespace gui
{
  class CachedTextView : public Element
  {
    strings::UniString m_uniText;

    vector<shared_ptr<graphics::DisplayList> > m_dls;
    vector<shared_ptr<graphics::DisplayList> > m_maskedDls;

    unique_ptr<graphics::GlyphLayout> m_layout;
    unique_ptr<graphics::GlyphLayout> m_maskedLayout;

  public:
    struct Params : public Element::Params
    {
      string m_text;
    };

    CachedTextView(Params const & p);

    void setText(string const & text);

    /// @name Overrider from graphics::OverlayElement and gui::Element.
    //@{
    virtual void GetMiniBoundRects(RectsT & rects) const;
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void cache();
    void purge();
    void layout();

    void setFont(EState state, graphics::FontDesc const & desc);

    void setPivot(m2::PointD const & pv);
    //@}
  };
}
