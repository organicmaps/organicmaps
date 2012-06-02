#pragma once

#include "../std/vector.hpp"

#include "../geometry/any_rect2d.hpp"

#include "../yg/straight_text_element.hpp"

#include "element.hpp"


namespace gui
{
  class TextView : public Element
  {
  private:

    shared_ptr<yg::StraightTextElement> m_elem;

    string m_text;

    void cache();

    mutable vector<m2::AnyRectD> m_boundRects;

  public:

    struct Params : public gui::Element::Params
    {
      string m_text;
    };

    TextView(Params const & p);

    void setText(string const & text);
    string const & text() const;

    yg::OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
    vector<m2::AnyRectD> const & boundRects() const;
    void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m);

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);
  };
}
