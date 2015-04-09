#pragma once

#include "gui/element.hpp"

#include "std/unique_ptr.hpp"


namespace graphics
{
  class DisplayList;
  class StraightTextElement;
}

namespace gui
{
  class TextView : public Element
  {
    map<EState, unique_ptr<graphics::DisplayList> > m_dls;

    typedef map<EState, unique_ptr<graphics::StraightTextElement> > ElemsMapT;
    ElemsMapT m_elems;

    string m_text;
    unsigned m_maxWidth;

    void cacheBody(EState state);
    void layoutBody(EState state);

  public:
    struct Params : public Element::Params
    {
      string m_text;
    };

    TextView(Params const & p);

    void setText(string const & text);
    string const & text() const;
    void setMaxWidth(unsigned width);

    /// @name Overrider from graphics::OverlayElement and gui::Element.
    //@{
    virtual void GetMiniBoundRects(RectsT & rects) const;

    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;

    void cache();
    void purge();
    void layout();

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);
    //@}
  };
}
