#pragma once

#include "graphics/overlay_element.hpp"
#include "graphics/color.hpp"
#include "graphics/font_desc.hpp"

#include "std/map.hpp"


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
      EInactive = 0,
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
    //@{
    virtual bool onTapStarted(m2::PointD const & pt);
    virtual bool onTapMoved(m2::PointD const & pt);
    virtual bool onTapEnded(m2::PointD const & pt);
    virtual bool onTapCancelled(m2::PointD const & pt);
    //@}

    /// Invalidate the rendering system to redraw the gui elements.
    void invalidate();
    /// obtain @see VisualScale
    double visualScale() const;

    /// This method is called to cache visual appearance of gui::Element for fast rendering.
    /// It should be called after layout() is calculated properly.
    virtual void cache();
    /// This method is called upon renderPolicy destruction and should clean
    /// all rendering-related resources, p.e. displayLists.
    virtual void purge();
    /// This method is called in each frame and should be overriden if the
    /// element wants to update it's internal state.
    virtual void update();
    /// This method is called after gui::Controller::SetRenderParams to
    /// perform layout calculations which might depends on RenderParams.
    /// It should be called when isDirtyLayout is set to true (visual parameters of object is changed).
    virtual void layout();
    /// Set the parent controller for this element. Should be called for all inner Elemen's too.
    virtual void setController(Controller * controller);

    /// Check if the layout of element is dirty and re-layout element if needed.
    /// Used in a "lazy" layout/cache strategy (called before actual drawing).
    void checkDirtyLayout() const;

    /// @name Override from OverlayElement.
    //@{
    void draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const;
    double priority() const;
    void setTransformation(const math::Matrix<double, 3, 3> & m);
    //@}
  };
}
