#include "element.hpp"
#include "controller.hpp"

#include "../base/logging.hpp"


namespace gui
{
  Element::Element(Params const & p)
    : OverlayElement(p),
      m_controller(0),
      m_state(EActive)
  {}

  void Element::setState(EState state)
  {
    m_state = state;
  }

  Element::EState Element::state() const
  {
    return m_state;
  }

  void Element::setFont(EState state, graphics::FontDesc const & font)
  {
    m_fonts[state] = font;
  }

  graphics::FontDesc const & Element::font(EState state) const
  {
    return m_fonts[state];
  }

  void Element::setColor(EState state, graphics::Color const & c)
  {
    m_colors[state] = c;
  }

  graphics::Color const & Element::color(EState state) const
  {
    return m_colors[state];
  }

  void Element::invalidate()
  {
    if (m_controller)
      m_controller->Invalidate();
    else
      LOG(LDEBUG/*LWARNING*/, ("unattached gui::Element couldn't be invalidated!"));
  }

  double Element::visualScale() const
  {
    if (m_controller)
      return m_controller->GetVisualScale();
    else
    {
      LOG(LDEBUG/*LWARNING*/, ("unattached gui::Elements shouldn't call gui::Element::visualScale function"));
      return 0.0;
    }
  }

  void Element::cache()
  {}

  void Element::purge()
  {}

  void Element::update()
  {}

  void Element::layout()
  {}

  void Element::checkDirtyLayout() const
  {
    if (isDirtyLayout())
    {
      const_cast<Element*>(this)->layout();
      const_cast<Element*>(this)->cache();
      setIsDirtyLayout(false);
    }
  }

  void Element::draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const &) const
  {
    DrawRectsDebug(r, color(state()), depth());
  }

  double Element::priority() const
  {
    return 0;
  }

  bool Element::onTapStarted(m2::PointD const & pt)
  {
    return false;
  }

  bool Element::onTapMoved(m2::PointD const & pt)
  {
    return false;
  }

  bool Element::onTapEnded(m2::PointD const & pt)
  {
    return false;
  }

  bool Element::onTapCancelled(m2::PointD const & pt)
  {
    return false;
  }

  void Element::setController(Controller * controller)
  {
    m_controller = controller;
  }

  void Element::setTransformation(const math::Matrix<double, 3, 3> & /*m*/)
  {
  }
}
