#include "controller.hpp"
#include "element.hpp"

#include "../map/drawer.hpp"

#include "../graphics/overlay.hpp"

#include "../std/bind.hpp"

namespace gui
{
  Controller::RenderParams::RenderParams()
    : m_VisualScale(0), m_GlyphCache(0)
  {}

  Controller::Controller()
    : m_VisualScale(0), m_GlyphCache(0)
  {}

  Controller::RenderParams::RenderParams(double visualScale,
                                         TInvalidateFn invalidateFn,
                                         graphics::GlyphCache * glyphCache,
                                         graphics::Screen * cacheScreen)
    : m_VisualScale(visualScale),
      m_InvalidateFn(invalidateFn),
      m_GlyphCache(glyphCache),
      m_CacheScreen(cacheScreen)
  {}

  Controller::~Controller()
  {}

  void Controller::SelectElements(m2::PointD const & pt, elem_list_t & l, bool onlyVisible)
  {
    for (elem_list_t::const_iterator it = m_Elements.begin();
         it != m_Elements.end();
         ++it)
    {
      shared_ptr<gui::Element> const & e = *it;
      if ((!onlyVisible || e->isVisible()) && e->roughHitTest(pt) && e->hitTest(pt))
        l.push_back(e);
    }
  }

  bool Controller::OnTapStarted(m2::PointD const & pt)
  {
    elem_list_t l;

    SelectElements(pt, l, true);

    /// selecting first hit-tested element from the list
    if (!l.empty())
    {
      m_focusedElement = l.front();
      m_focusedElement->onTapStarted(pt);
      m_LastTapCancelled = false;
      return true;
    }

    return false;
  }

  bool Controller::OnTapMoved(m2::PointD const & pt)
  {
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

      /// event handled
      return true;
    }

    return false;
  }

  bool Controller::OnTapEnded(m2::PointD const & pt)
  {
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
    elem_list_t::iterator it = find(m_Elements.begin(), m_Elements.end(), e);

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

  void Controller::SetRenderParams(RenderParams const & p)
  {
    m_GlyphCache = p.m_GlyphCache;
    m_InvalidateFn = p.m_InvalidateFn;
    m_VisualScale = p.m_VisualScale;
    m_CacheScreen = p.m_CacheScreen;

    LayoutElements();
  }

  void Controller::ResetRenderParams()
  {
    m_GlyphCache = 0;
    m_VisualScale = 0;
    m_InvalidateFn.clear();
    m_CacheScreen = 0;

    PurgeElements();
  }

  void Controller::DrawFrame(graphics::Screen * screen)
  {
    screen->beginFrame();

    math::Matrix<double, 3, 3> m = math::Identity<double, 3>();

    for (elem_list_t::const_iterator it = m_Elements.begin();
         it != m_Elements.end();
         ++it)
      (*it)->draw(screen, m);

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

    for (elem_list_t::const_iterator it = m_Elements.begin();
         it != m_Elements.end();
         ++it)
      (*it)->setIsDirtyLayout(true);
  }

  StringsBundle const * Controller::GetStringsBundle() const
  {
    return m_bundle;
  }

  graphics::Screen * Controller::GetCacheScreen() const
  {
    return m_CacheScreen;
  }

  void Controller::UpdateElements()
  {
    for (elem_list_t::const_iterator it = m_Elements.begin();
         it != m_Elements.end();
         ++it)
      (*it)->update();
  }

  void Controller::PurgeElements()
  {
    for (elem_list_t::const_iterator it = m_Elements.begin();
         it != m_Elements.end();
         ++it)
    {
      (*it)->purge();
      (*it)->setIsDirtyLayout(true);
    }
  }

  void Controller::LayoutElements()
  {
    for (elem_list_t::const_iterator it = m_Elements.begin();
         it != m_Elements.end();
         ++it)
      (*it)->checkDirtyLayout();
  }
}
