#include "controller.hpp"
#include "element.hpp"

#include "../map/drawer_yg.hpp"

#include "../yg/overlay.hpp"

#include "../std/bind.hpp"

namespace gui
{
  Controller::RenderParams::RenderParams()
    : m_VisualScale(0), m_GlyphCache(0)
  {}

  Controller::Controller()
    : m_VisualScale(0), m_GlyphCache(0), m_IsAttached(false)
  {}

  Controller::RenderParams::RenderParams(double visualScale,
                                         TInvalidateFn invalidateFn,
                                         yg::GlyphCache * glyphCache)
    : m_VisualScale(visualScale),
      m_InvalidateFn(invalidateFn),
      m_GlyphCache(glyphCache)
  {}

  Controller::~Controller()
  {}

  void Controller::SelectElements(m2::PointD const & pt, elem_list_t & l)
  {
    base_list_t ll;
    m_Overlay.selectOverlayElements(pt, ll);
    for (base_list_t::const_iterator it = ll.begin(); it != ll.end(); ++it)
      if ((*it)->roughHitTest(pt) && (*it)->hitTest(pt))
        l.push_back(boost::static_pointer_cast<Element>(*it));
  }

  bool Controller::OnTapStarted(m2::PointD const & pt)
  {
    elem_list_t l;

    SelectElements(pt, l);

    /// selecting first hit-tested element from the list
    if (!l.empty())
    {
      m_focusedElement = l.front();
      m_focusedElement->onTapStarted(pt);
      return true;
    }

    return false;
  }

  bool Controller::OnTapMoved(m2::PointD const & pt)
  {
    if (m_focusedElement)
    {
      if (!m_focusedElement->roughHitTest(pt) || !m_focusedElement->hitTest(pt))
        m_focusedElement->onTapCancelled(pt);
      else
        m_focusedElement->onTapMoved(pt);

      /// event handled
      return true;
    }

    return false;
  }

  bool Controller::OnTapEnded(m2::PointD const & pt)
  {
    if (m_focusedElement)
    {
      m_focusedElement->onTapEnded(pt);
      m_focusedElement.reset();

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

  struct FindByPointer
  {
    yg::OverlayElement const * m_e;
    shared_ptr<yg::OverlayElement> * m_res;
    FindByPointer(yg::OverlayElement const * e,
                  shared_ptr<yg::OverlayElement> * res) : m_e(e), m_res(res)
    {}

    void operator()(shared_ptr<yg::OverlayElement> const & oe)
    {
      if (m_e == oe.get())
        *m_res = oe;
    }
  };

  shared_ptr<Element> const Controller::FindElement(Element const * e)
  {
    shared_ptr<Element> res;
    shared_ptr<yg::OverlayElement> resBase;
    FindByPointer fn(e, &resBase);

    m_Overlay.forEach(fn);

    if (fn.m_res)
      res = boost::static_pointer_cast<Element>(resBase);

    return res;
  }

  void Controller::RemoveElement(shared_ptr<Element> const & e)
  {
    TElementRects::const_iterator it = m_ElementRects.find(e);

    if (it != m_ElementRects.end())
      m_Overlay.removeOverlayElement(e, it->second);

    e->setController(0);
  }

  void Controller::AddElement(shared_ptr<Element> const & e)
  {
    e->setController(this);

    if (m_IsAttached)
    {
      m_ElementRects[e] = e->roughBoundRect();
      m_Overlay.processOverlayElement(e);
    }
    else
      m_RawElements.push_back(e);
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

    m_IsAttached = true;

    base_list_t l;
    m_Overlay.forEach(MakeBackInsertFunctor(l));

    copy(m_RawElements.begin(), m_RawElements.end(), back_inserter(l));
    m_RawElements.clear();

    m_Overlay.clear();

    for (base_list_t::const_iterator it = l.begin();
         it != l.end();
         ++it)
    {
      (*it)->setIsDirtyRect(true);
      m_ElementRects[*it] = (*it)->roughBoundRect();
      m_Overlay.processOverlayElement(*it);
    }
  }

  void Controller::ResetRenderParams()
  {
    m_GlyphCache = 0;
    m_VisualScale = 0;
    m_InvalidateFn.clear();
    m_IsAttached = false;

    base_list_t l;

    m_Overlay.forEach(MakeBackInsertFunctor(l));

    for (base_list_t::const_iterator it = l.begin();
         it != l.end();
         ++it)
      m_RawElements.push_back(boost::static_pointer_cast<Element>(*it));

    m_Overlay.clear();
    m_ElementRects.clear();
  }

  void Controller::DrawFrame(yg::gl::Screen * screen)
  {
    screen->beginFrame();

    math::Matrix<double, 3, 3> m = math::Identity<double, 3>();

    m_Overlay.draw(screen, m);

    screen->endFrame();
  }

  void Controller::Invalidate()
  {
    if (m_InvalidateFn)
      m_InvalidateFn();
  }

  yg::GlyphCache * Controller::GetGlyphCache() const
  {
    return m_GlyphCache;
  }
}
