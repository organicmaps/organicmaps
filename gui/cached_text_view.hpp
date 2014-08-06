#pragma once

#include "element.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

#include "../base/string_utils.hpp"
#include "../base/matrix.hpp"

#include "../graphics/glyph_cache.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/glyph_layout.hpp"

namespace gui
{
  class CachedTextView : public Element
  {
  private:

    string m_text;

    strings::UniString m_uniText;

    vector<shared_ptr<graphics::DisplayList> > m_dls;
    vector<shared_ptr<graphics::DisplayList> > m_maskedDls;

    unique_ptr<graphics::GlyphLayout> m_layout;
    unique_ptr<graphics::GlyphLayout> m_maskedLayout;

    mutable vector<m2::AnyRectD> m_boundRects;

  public:

    struct Params : public Element::Params
    {
      string m_text;
    };

    CachedTextView(Params const & p);

    void setText(string const & text);
    string const & text() const;

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void cache();
    void purge();
    void layout();

    void setFont(EState state, graphics::FontDesc const & desc);

    void setPivot(m2::PointD const & pv);
  };
}
