#include "controller.hpp"
#include "element.hpp"

#include "../map/drawer_yg.hpp"

#include "../yg/overlay.hpp"

#include "../std/bind.hpp"

namespace gui
{
  Controller::Controller() : m_VisualScale(1.0)
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

  void Controller::AddElement(shared_ptr<Element> const & e)
  {
    m_Overlay.processOverlayElement(e);
    e->m_controller = this;
  }

  void Controller::UpdateElement(Element * e)
  {
    m_Overlay.updateOverlayElement(e);
  }

  double Controller::VisualScale() const
  {
    return m_VisualScale;
  }

  void Controller::SetVisualScale(double val)
  {
    m_VisualScale = val;
    m_Overlay.forEach(bind(&yg::OverlayElement::setIsDirtyRect, _1, true));
  }

  void Controller::DrawFrame(yg::gl::Screen * screen)
  {
    screen->beginFrame();

    math::Matrix<double, 3, 3> m = math::Identity<double, 3>();

    m_Overlay.draw(screen, m);

    screen->endFrame();
  }

  void Controller::SetInvalidateFn(TInvalidateFn fn)
  {
    m_InvalidateFn = fn;
  }

  void Controller::ResetInvalidateFn()
  {
    m_InvalidateFn.clear();
  }

  void Controller::Invalidate()
  {
    if (m_InvalidateFn)
      m_InvalidateFn();
  }
}
