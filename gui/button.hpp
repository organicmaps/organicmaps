#pragma once

#include "element.hpp"

#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/unique_ptr.hpp"


namespace graphics
{
  class OverlayElement;
  class DisplayList;

  namespace gl
  {
    class OverlayRenderer;
  }
}

namespace gui
{
  class TextView;

  class Button : public Element
  {
  public:
    typedef function<void (Element const *)> TOnClickListener;

  private:
    TOnClickListener m_OnClickListener;

    unsigned m_minWidth;
    unsigned m_minHeight;

    unique_ptr<TextView> m_textView;
    map<EState, unique_ptr<graphics::DisplayList> > m_dls;

    void cacheButtonBody(EState state);

  public:
    struct Params : Element::Params
    {
      unsigned m_minWidth;
      unsigned m_minHeight;
      string m_text;
      Params();
    };

    Button(Params const & p);

    void setOnClickListener(TOnClickListener const & l);

    void setFont(EState state, graphics::FontDesc const & font);
    void setColor(EState state, graphics::Color const & c);

    void setText(string const & text);
    string const & text() const;

    void setMinWidth(unsigned minWidthInDIP);
    unsigned minWidth() const;

    void setMinHeight(unsigned minHeightInDIP);
    unsigned minHeight() const;

    /// @name Override from graphics::OverlayElement and gui::Element.
    //@{
    virtual m2::RectD GetBoundRect() const;

    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    void setPivot(m2::PointD const & pv);

    void purge();
    void layout();
    void cache();

    void setController(Controller * controller);

    bool onTapStarted(m2::PointD const & pt);
    bool onTapMoved(m2::PointD const & pt);
    bool onTapEnded(m2::PointD const & pt);
    bool onTapCancelled(m2::PointD const & pt);
    //@}
  };
}
