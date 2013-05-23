#pragma once

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

#include "../geometry/any_rect2d.hpp"

#include "../graphics/straight_text_element.hpp"
#include "../graphics/display_list.hpp"

#include "element.hpp"

namespace gui
{
  class TextView : public Element
  {
  private:

    map<EState, shared_ptr<graphics::DisplayList> > m_dls;
    map<EState, shared_ptr<graphics::StraightTextElement> > m_elems;

    string m_text;
    unsigned m_maxWidth;

    mutable vector<m2::AnyRectD> m_boundRects;

    void cacheBody(EState state);
    void layoutBody(EState state);

  public:

    void cache();
    void purge();
    void layout();

    struct Params : public Element::Params
    {
      string m_text;
    };

    TextView(Params const & p);

    void setText(string const & text);
    string const & text() const;
    void setMaxWidth(unsigned width);

    vector<m2::AnyRectD> const & boundRects() const;
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);
  };
}
