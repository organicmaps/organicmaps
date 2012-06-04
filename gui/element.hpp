#pragma once

#include "../geometry/point2d.hpp"

#include "../yg/overlay_element.hpp"
#include "../yg/color.hpp"
#include "../yg/font_desc.hpp"

#include "../std/map.hpp"

namespace yg
{
  namespace gl
  {
    class OverlayRenderer;
  }
}

namespace gui
{
  class Controller;

  class Element : public yg::OverlayElement
  {
  public:

    enum EState
    {
      EInactive,
      EActive,
      EPressed,
      ESelected
    };

  protected:

    Controller * m_controller;

  private:

    EState m_state;

    mutable map<EState, yg::FontDesc> m_fonts;
    mutable map<EState, yg::Color> m_colors;

  public:

    typedef OverlayElement::Params Params;

    Element(Params const & p);

    void setState(EState state);
    EState state() const;

    void setFont(EState state, yg::FontDesc const & font);
    yg::FontDesc const & font(EState state) const;

    void setColor(EState state, yg::Color const & c);
    yg::Color const & color(EState state) const;

    /// Implement this method to handle single tap on the GUI element.
    virtual bool onTapStarted(m2::PointD const & pt);
    virtual bool onTapMoved(m2::PointD const & pt);
    virtual bool onTapEnded(m2::PointD const & pt);
    virtual bool onTapCancelled(m2::PointD const & pt);

    void invalidate();
    double visualScale() const;

    yg::OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
    void draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    int visualRank() const;

    virtual void cache();

    virtual void setController(Controller * controller);

    void checkDirtyDrawing() const;
  };
}
