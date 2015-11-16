#pragma once

#include "gui/display_list_cache.hpp"

#include "graphics/defines.hpp"

#include "geometry/point2d.hpp"

#include "base/strings_bundle.hpp"

#include "std/shared_ptr.hpp"
#include "std/unique_ptr.hpp"
#include "std/function.hpp"
#include "std/list.hpp"


namespace graphics
{
  class GlyphCache;
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

    struct RenderParams
    {
      graphics::EDensity m_Density;
      int m_exactDensityDPI;
      TInvalidateFn m_InvalidateFn;
      graphics::GlyphCache * m_GlyphCache;
      graphics::Screen * m_CacheScreen;
      RenderParams();
      RenderParams(graphics::EDensity density,
                   int exactDensityDPI,
                   TInvalidateFn invalidateFn,
                   graphics::GlyphCache * glyphCache,
                   graphics::Screen * cacheScreen);
    };

  private:

    /// Element that has focus.
    shared_ptr<Element> m_focusedElement;

    typedef list<shared_ptr<Element> > ElemsT;

    ElemsT m_Elements;

    /// Select top element under specified point for tap processing.
    shared_ptr<Element> SelectTopElement(m2::PointD const & pt, bool onlyVisible) const;

    /// Invalidate GUI function
    TInvalidateFn m_InvalidateFn;

    /// Screen density
    graphics::EDensity m_Density;

    /// VisualScale to multiply all Device-Independent-Pixels dimensions.
    double m_VisualScale;

    /// GlyphCache for text rendering by GUI elements.
    graphics::GlyphCache * m_GlyphCache;

    /// Cache for display lists for fast rendering on GUI thread
    unique_ptr<DisplayListCache> m_DisplayListCache;

    /// Localized strings for GUI.
    StringsBundle const * m_bundle;

    /// Screen, which is used to cache gui::Elements into display lists.
    graphics::Screen * m_CacheScreen = nullptr;

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
    /// Get Density parameter
    graphics::EDensity GetDensity() const;
    /// Get localized strings bundle
    StringsBundle const * GetStringsBundle() const;
    /// Get GlyphCache
    graphics::GlyphCache * GetGlyphCache() const;
    /// Get graphics::Screen, which is used to cache gui::Element's
    /// into display lists.
    graphics::Screen * GetCacheScreen() const;
    /// Get display list cache
    DisplayListCache * GetDisplayListCache() const;
    /// Redraw GUI
    void DrawFrame(graphics::Screen * screen);
    /// Calling gui::Element::update for every element.
    void UpdateElements();
    /// Calling gui::Element::purge for every element.
    void PurgeElements();
    /// Calling gui::Element::performLayout for every element
    void LayoutElements();
  };
}
