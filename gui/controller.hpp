#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"
#include "../std/list.hpp"
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
    typedef list<shared_ptr<Element> > elem_list_t;

    elem_list_t m_Elements;

    /// select elements under specified point
    void SelectElements(m2::PointD const & pt, elem_list_t & l);

    /// Invalidate GUI function
    TInvalidateFn m_InvalidateFn;

    /// VisualScale to multiply all Device-Independent-Pixels dimensions.
    double m_VisualScale;

    /// GlyphCache for text rendering by GUI elements.
    yg::GlyphCache * m_GlyphCache;

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
