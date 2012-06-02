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

    typedef list<shared_ptr<Element> > elem_list_t;

    /// Temporary list to store gui::Element's before the AttachRenderer call.
    /// As the gui::Elements could use the Controller::RendererParams in the bounding
    /// rects calculation (for example GlyphCache for gui::TextView or VisualScale
    /// for almost every element), we shouldn't call boundRects() function
    /// before AttachRenderer. This implies that we couldn't add gui::Element to the
    /// yg::Overlay correctly, as the m4::Tree::Add function use gui::Element::roughBoundRect
    /// We'll add this elements into yg::Overlay in AttachRenderer function.
    elem_list_t m_RawElements;

    /// select elements under specified point
    void SelectElements(m2::PointD const & pt, elem_list_t & l);

    /// Invalidate GUI function
    TInvalidateFn m_InvalidateFn;

    /// VisualScale to multiply all Device-Independent-Pixels dimensions.
    double m_VisualScale;

    /// GlyphCache for text rendering by GUI elements.
    yg::GlyphCache * m_GlyphCache;

    /// Is this controller attached to the renderer?
    bool m_IsAttached;

    /// For fast removing of gui::Element's upon element geometry change
    typedef map<shared_ptr<yg::OverlayElement>, m2::RectD> TElementRects;
    TElementRects m_ElementRects;

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

    /// Controller should be attached to the renderer before
    /// rendering GUI elements. Usually it's done after
    /// Framework::SetRenderPolicy
    void AttachToRenderer();
    /// Controller should be detached from the renderer
    /// when we are about to finish all rendering.
    void DetachFromRenderer();

    struct RenderParams
    {
      double m_VisualScale;
      TInvalidateFn m_InvalidateFn;
      yg::GlyphCache * m_GlyphCache;
      RenderParams();
      RenderParams(double visualScale,
                   TInvalidateFn invalidateFn,
                   yg::GlyphCache * glyphCache);
    };

    /// Attach GUI Controller to the renderer
    void SetRenderParams(RenderParams const & p);
    /// Detach GUI Controller from the renderer
    void ResetRenderParams();
    /// Invalidate the scene
    void Invalidate();
    /// Find shared_ptr from the pointer in m_Overlay
    shared_ptr<Element> const FindElement(Element const * e);
    /// Remove GUI element by pointer
    void RemoveElement(shared_ptr<Element> const & e);
    /// Add GUI element to the controller
    void AddElement(shared_ptr<Element> const & e);
    /// Get VisualScale parameter
    double GetVisualScale() const;
    /// Get GLyphCache
    yg::GlyphCache * GetGlyphCache() const;
    /// Redraw GUI
    void DrawFrame(yg::gl::Screen * screen);
  };
}
