#include "gui/controller.hpp"
#include "gui/element.hpp"

#include "graphics/screen.hpp"

#include "std/bind.hpp"


namespace gui
{
  Controller::RenderParams::RenderParams()
    : m_Density(graphics::EDensityMDPI), m_exactDensityDPI(0), m_GlyphCache(0)
  {}

  Controller::Controller()
    : m_Density(graphics::EDensityMDPI), m_GlyphCache(0)
  {}

  Controller::RenderParams::RenderParams(graphics::EDensity density,
                                         int exactDensityDPI,
                                         TInvalidateFn invalidateFn,
                                         graphics::GlyphCache * glyphCache,
                                         graphics::Screen * cacheScreen)
    : m_Density(density),
      m_exactDensityDPI(exactDensityDPI),
      m_InvalidateFn(invalidateFn),
      m_GlyphCache(glyphCache),
      m_CacheScreen(cacheScreen)
  {}

  Controller::~Controller()
  {}

  shared_ptr<Element> Controller::SelectTopElement(m2::PointD const & pt, bool onlyVisible) const
  {
    shared_ptr<Element> res;

    for (ElemsT::const_iterator it = m_Elements.begin(); it != m_Elements.end(); ++it)
    {
      shared_ptr<gui::Element> const & e = *it;
      if ((!onlyVisible || e->isVisible()) && e->hitTest(pt))
      {
        if (!res || e->depth() > res->depth())
          res = e;
      }
    }

    return res;
  }

  bool Controller::OnTapStarted(m2::PointD const & pt)
  {
    if (GetCacheScreen() == nullptr)
      return false;

    shared_ptr<Element> e = SelectTopElement(pt, true);
    if (e)
    {
      m_focusedElement = e;
      m_focusedElement->onTapStarted(pt);
      m_LastTapCancelled = false;
      return true;
    }

    return false;
  }

  bool Controller::OnTapMoved(m2::PointD const & pt)
  {
    if (GetCacheScreen() == nullptr)
      return false;

    if (m_focusedElement)
    {
      if (!m_LastTapCancelled)
      {
        if (!m_focusedElement->hitTest(pt))
        {
          m_focusedElement->onTapCancelled(pt);
          m_LastTapCancelled = true;
        }
        else
          m_focusedElement->onTapMoved(pt);
      }

      // event handled
      return true;
    }

    return false;
  }

  bool Controller::OnTapEnded(m2::PointD const & pt)
  {
    if (GetCacheScreen() == nullptr)
      return false;

    if (m_focusedElement)
    {
      // re-checking, whether we are above the gui element.
      if (!m_LastTapCancelled)
      {
        if (!m_focusedElement->hitTest(pt))
        {
          m_focusedElement->onTapCancelled(pt);
          m_LastTapCancelled = true;
        }
        else
          m_focusedElement->onTapEnded(pt);
      }

      m_focusedElement.reset();
      m_LastTapCancelled = false;

      return true;
    }

    return false;
  }

  bool Controller::OnTapCancelled(m2::PointD const & pt)
  {
    if (GetCacheScreen() == nullptr)
      return false;

    if (m_focusedElement)
    {
      m_focusedElement->onTapCancelled(pt);
      m_focusedElement.reset();
      return true;
    }

    return false;
  }

  void Controller::RemoveElement(shared_ptr<Element> const & e)
  {
    ElemsT::iterator it = find(m_Elements.begin(), m_Elements.end(), e);

    if (it != m_Elements.end())
      m_Elements.erase(it);

    e->setController(0);
  }

  void Controller::AddElement(shared_ptr<Element> const & e)
  {
    e->setController(this);
    m_Elements.push_back(e);
  }

  double Controller::GetVisualScale() const
  {
    return m_VisualScale;
  }

  graphics::EDensity Controller::GetDensity() const
  {
    return m_Density;
  }

  void Controller::SetRenderParams(RenderParams const & p)
  {
    m_GlyphCache = p.m_GlyphCache;
    m_InvalidateFn = p.m_InvalidateFn;
    m_Density = p.m_Density;
    m_VisualScale = graphics::visualScaleExact(p.m_exactDensityDPI);
    m_CacheScreen = p.m_CacheScreen;

    m_DisplayListCache.reset(new DisplayListCache(m_CacheScreen, m_GlyphCache));

    LayoutElements();
  }

  void Controller::ResetRenderParams()
  {
    m_GlyphCache = 0;
    m_Density = graphics::EDensityLDPI;
    m_InvalidateFn = TInvalidateFn();
    m_CacheScreen = nullptr;

    PurgeElements();

    m_DisplayListCache.reset();
  }

  void Controller::DrawFrame(graphics::Screen * screen)
  {
    if (m_CacheScreen == nullptr)
      return;

    screen->beginFrame();

    math::Matrix<double, 3, 3> const m = math::Identity<double, 3>();
    for (auto const & element : m_Elements)
      element->draw(screen, m);

    screen->endFrame();
  }

  void Controller::Invalidate()
  {
    if (m_InvalidateFn)
      m_InvalidateFn();
  }

  graphics::GlyphCache * Controller::GetGlyphCache() const
  {
    return m_GlyphCache;
  }

  void Controller::SetStringsBundle(StringsBundle const * bundle)
  {
    m_bundle = bundle;
    for (auto const & element : m_Elements)
      element->setIsDirtyLayout(true);
  }

  StringsBundle const * Controller::GetStringsBundle() const
  {
    return m_bundle;
  }

  graphics::Screen * Controller::GetCacheScreen() const
  {
    return m_CacheScreen;
  }

  DisplayListCache * Controller::GetDisplayListCache() const
  {
    return m_DisplayListCache.get();
  }

  void Controller::UpdateElements()
  {
    for (auto const & element : m_Elements)
      element->update();
  }

  void Controller::PurgeElements()
  {
    for (auto const & element : m_Elements)
    {
      element->purge();
      element->setIsDirtyLayout(true);
    }
  }

  void Controller::LayoutElements()
  {
    for (auto const & element : m_Elements)
      element->checkDirtyLayout();
  }
}
