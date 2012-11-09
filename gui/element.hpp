#pragma once

#include "../geometry/point2d.hpp"

#include "../graphics/overlay_element.hpp"
#include "../graphics/color.hpp"
#include "../graphics/font_desc.hpp"

#include "../std/map.hpp"

namespace graphics
{
  namespace gl
  {
    class OverlayRenderer;
  }
}

namespace gui
{
  class Controller;

  class Element : public graphics::OverlayElement
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

    mutable map<EState, graphics::FontDesc> m_fonts;
    mutable map<EState, graphics::Color> m_colors;

  public:

    typedef OverlayElement::Params Params;

    Element(Params const & p);

    void setState(EState state);
    EState state() const;

    virtual void setFont(EState state, graphics::FontDesc const & font);
    graphics::FontDesc const & font(EState state) const;

    virtual void setColor(EState state, graphics::Color const & c);
    graphics::Color const & color(EState state) const;

    /// Implement this method to handle single tap on the GUI element.
    virtual bool onTapStarted(m2::PointD const & pt);
    virtual bool onTapMoved(m2::PointD const & pt);
    virtual bool onTapEnded(m2::PointD const & pt);
    virtual bool onTapCancelled(m2::PointD const & pt);

    void invalidate();
    double visualScale() const;

    graphics::OverlayElement * clone(math::Matrix<double, 3, 3> const & m) const;
    void draw(graphics::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    int visualRank() const;

    virtual void cache();
    /// this method is called upon renderPolicy destruction and should clean
    /// all rendering-related resources, p.e. displayLists.
    virtual void purge();
    /// this method is called in each frame and should be overriden if the
    /// element wants to update it's internal state.
    virtual void update();

    virtual void setController(Controller * controller);

    void checkDirtyDrawing() const;
  };
}
