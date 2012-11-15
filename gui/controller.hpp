#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"
#include "../std/list.hpp"

#include "../geometry/point2d.hpp"

#include "../base/strings_bundle.hpp"

namespace graphics
{
  class GlyphCache;
  class OverlayElement;
  class Screen;
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

    typedef list<shared_ptr<graphics::OverlayElement> > base_list_t;
    typedef list<shared_ptr<Element> > elem_list_t;

    elem_list_t m_Elements;

    /// select elements under specified point
    void SelectElements(m2::PointD const & pt, elem_list_t & l, bool onlyVisible);

    /// Invalidate GUI function
    TInvalidateFn m_InvalidateFn;

    /// VisualScale to multiply all Device-Independent-Pixels dimensions.
    double m_VisualScale;

    /// GlyphCache for text rendering by GUI elements.
    graphics::GlyphCache * m_GlyphCache;

    /// Localized strings for GUI.
    StringsBundle const * m_bundle;

    /// Screen, which is used to cache gui::Elements into display lists.
    graphics::Screen * m_CacheScreen;

    /// Should we call the onTapEnded when the tap finished(we should
    /// not if the tap was cancelled while moving).
    bool m_LastTapCancelled;

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
      graphics::GlyphCache * m_GlyphCache;
      graphics::Screen * m_CacheScreen;
      RenderParams();
      RenderParams(double visualScale,
                   TInvalidateFn invalidateFn,
                   graphics::GlyphCache * glyphCache,
                   graphics::Screen * cacheScreen);
    };

    /// Attach GUI Controller to the renderer
    void SetRenderParams(RenderParams const & p);
    /// Set the bundle with localized strings
    void SetStringsBundle(StringsBundle const * bundle);
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
    /// Get localized strings bundle
    StringsBundle const * GetStringsBundle() const;
    /// Get GlyphCache
    graphics::GlyphCache * GetGlyphCache() const;
    /// Get graphics::Screen, which is used to cache gui::Element's
    /// into display lists.
    graphics::Screen * GetCacheScreen() const;
    /// Redraw GUI
    void DrawFrame(graphics::Screen * screen);
    /// Calling gui::Element::update for every element.
    void UpdateElements();
    /// Calling gui::Element::purge for every element.
    void PurgeElements();
  };
}
