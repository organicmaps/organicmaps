#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"
#include "../yg/overlay.hpp"

namespace yg
{
  namespace gl
  {
    class Screen;
  }
}

namespace gui
{
  class Element;

  /// Controller for GUI elements, which tracks mouse, keyboard and
  /// touch user interactions into interactions with GUI elements.
  class Controller
  {
  public:

    /// Invalidate functor type
    typedef function<void()> TInvalidateFn;

  private:

    /// element that has focus.
    shared_ptr<Element> m_focusedElement;

    typedef list<shared_ptr<yg::OverlayElement> > base_list_t;

    /// Overlay, which holds all GUI elements.
    yg::Overlay m_Overlay;

    /// container for gui::Element's
    typedef list<shared_ptr<Element> > elem_list_t;

    /// select elements under specified point
    void SelectElements(m2::PointD const & pt, elem_list_t & l);

    /// Invalidate GUI function
    TInvalidateFn m_InvalidateFn;

    /// VisualScale to multiply all Device-Independent-Pixels dimensions.
    double m_VisualScale;

  public:

    /// Constructor with GestureDetector to route events from.
    Controller();
    /// Destructor
    virtual ~Controller();
    /// Handlers to be called from the client code to power up the GUI.
    /// @{
    bool OnTapStarted(m2::PointD const & pt);
    bool OnTapMoved(m2::PointD const & pt);
    bool OnTapEnded(m2::PointD const & pt);
    bool OnTapCancelled(m2::PointD const & pt);
    /// @}
    /// Set Invalidate functor
    void SetInvalidateFn(TInvalidateFn fn);
    /// Reset Invalidate functor
    void ResetInvalidateFn();
    /// Set VisualScale
    void SetVisualScale(double val);
    /// Invalidate the scene
    void Invalidate();
    /// Add GUI element to the controller
    void AddElement(shared_ptr<Element> const & e);
    /// Update element position in the Overlay, as it's coordinates might have changed.
    void UpdateElement(Element * e);
    /// Get VisualScale parameter
    double VisualScale() const;
    /// Redraw GUI
    void DrawFrame(yg::gl::Screen * screen);
  };
}
