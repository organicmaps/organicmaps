#include "element.hpp"
#include "controller.hpp"

#include "../yg/overlay_renderer.hpp"

#include "../base/logging.hpp"

namespace gui
{
  Element::Element(Params const & p)
    : OverlayElement(p),
      m_controller(0),
      m_state(EActive)
  {
  }

  void Element::setState(EState state)
  {
    m_state = state;
  }

  Element::EState Element::state() const
  {
    return m_state;
  }

  void Element::setFont(EState state, yg::FontDesc const & font)
  {
    m_fonts[state] = font;
  }

  yg::FontDesc const & Element::font(EState state) const
  {
    return m_fonts[state];
  }

  void Element::setColor(EState state, yg::Color const & c)
  {
    m_colors[state] = c;
  }

  yg::Color const & Element::color(EState state) const
  {
    return m_colors[state];
  }

  void Element::invalidate()
  {
    if (m_controller)
      m_controller->Invalidate();
    else
      LOG(LWARNING, ("unattached gui::Element couldn't be invalidated!"));
  }

  double Element::visualScale() const
  {
    if (m_controller)
      return m_controller->GetVisualScale();
    else
    {
      LOG(LWARNING, ("unattached gui::Elements shouldn't call gui::Element::visualScale function"));
      return 0.0;
    }
  }

  void Element::setPivot(m2::PointD const & pv)
  {
    shared_ptr<Element> e = m_controller->FindElement(this);

    Controller * controller = m_controller;

    if (e)
    {
      controller->RemoveElement(e);
      m_controller = controller;
    }

    OverlayElement::setPivot(pv);

    if (e)
      controller->AddElement(e);
  }

  void Element::cache()
  {}

  void Element::checkDirtyDrawing() const
  {
    if (isDirtyDrawing())
    {
      const_cast<Element*>(this)->cache();
      setIsDirtyDrawing(false);
    }
  }

  void Element::draw(yg::gl::OverlayRenderer *r, math::Matrix<double, 3, 3> const & m) const
  {
    for (unsigned i = 0; i < boundRects().size(); ++i)
      r->drawRectangle(boundRects()[i], color(state()), depth());
  }

  int Element::visualRank() const
  {
    return 0;
  }
}
