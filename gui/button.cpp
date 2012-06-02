#include "button.hpp"
#include "../yg/overlay_renderer.hpp"

namespace gui
{
  Button::Button(Params const & p) : Element(p)
  {
    setWidth(p.m_width);
    setHeight(p.m_height);
    setText(p.m_text);

    setFont(EActive, yg::FontDesc(12, yg::Color(0, 0, 0, 255)));
    setFont(EPressed, yg::FontDesc(12, yg::Color(0, 0, 0, 255)));

    setColor(EActive, yg::Color(yg::Color(192, 192, 192, 255)));
    setColor(EPressed, yg::Color(yg::Color(64, 64, 64, 255)));
  }

  void Button::setOnClickListener(TOnClickListener const & l)
  {
    m_OnClickListener = l;
  }

  bool Button::onTapStarted(m2::PointD const & pt)
  {
    setState(EPressed);
    invalidate();
    return false;
  }

  bool Button::onTapCancelled(m2::PointD const & pt)
  {
    setState(EActive);
    invalidate();
    return false;
  }

  bool Button::onTapEnded(m2::PointD const & pt)
  {
    setState(EActive);
    if (m_OnClickListener)
      m_OnClickListener(this);
    invalidate();
    return false;
  }

  bool Button::onTapMoved(m2::PointD const & pt)
  {
    invalidate();
    return false;
  }

  void Button::setText(string const & text)
  {
    m_text = text;
  }

  string const & Button::text() const
  {
    return m_text;
  }

  void Button::setWidth(unsigned widthInDIP)
  {
    m_widthInDIP = widthInDIP;
    invalidate();
  }

  unsigned Button::width() const
  {
    return m_widthInDIP;
  }

  void Button::setHeight(unsigned heightInDIP)
  {
    m_heightInDIP = heightInDIP;
    invalidate();
  }

  unsigned Button::height() const
  {
    return m_heightInDIP;
  }

  yg::OverlayElement * Button::clone(math::Matrix<double, 3, 3> const & m) const
  {
    return new Button(*this);
  }

  vector<m2::AnyRectD> const & Button::boundRects() const
  {
    if (isDirtyRect())
    {
      m_boundRects.clear();
      double k = visualScale();
      m2::RectD rc(0, 0, width() * k, height() * k);
      rc.Offset(tieRect(rc, math::Identity<double, 3>()));
      m_boundRects.push_back(m2::AnyRectD(rc));
      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  void Button::draw(yg::gl::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (!isVisible())
      return;

    double k = visualScale();

    m2::RectD rc(0, 0, width() * k, height() * k);
    rc.Offset(tieRect(rc, m));
    r->drawRoundedRectangle(rc, 10 * k, color(state()), depth() - 1);

    yg::FontDesc desc = font(state());
    desc.m_size *= k;

    r->drawText(desc, pivot(), position(), text(), depth(), false, false);
  }
}
