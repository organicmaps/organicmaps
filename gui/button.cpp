#include "button.hpp"
#include "controller.hpp"

#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"

#include "../geometry/transformations.hpp"


namespace gui
{
  Button::Params::Params()
    : m_minWidth(10),
      m_minHeight(10)
  {}

  Button::Button(Params const & p) : Element(p)
  {
    TextView::Params tp;

    tp.m_depth = p.m_depth + 1;
    tp.m_pivot = p.m_pivot;
    tp.m_position = graphics::EPosCenter;
    tp.m_text = p.m_text;

    m_textView.reset(new TextView(tp));

    setMinWidth(p.m_minWidth);
    setMinHeight(p.m_minHeight);
    setText(p.m_text);

    setFont(EActive, graphics::FontDesc(12, graphics::Color(0, 0, 0, 255)));
    setFont(EPressed, graphics::FontDesc(12, graphics::Color(0, 0, 0, 255)));

    setColor(EActive, graphics::Color(graphics::Color(192, 192, 192, 255)));
    setColor(EPressed, graphics::Color(graphics::Color(64, 64, 64, 255)));
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
    m_textView->setText(text);
  }

  string const & Button::text() const
  {
    return m_textView->text();
  }

  void Button::setMinWidth(unsigned minWidth)
  {
    m_minWidth = minWidth;
    invalidate();
  }

  unsigned Button::minWidth() const
  {
    return m_minWidth;
  }

  void Button::setMinHeight(unsigned minHeight)
  {
    m_minHeight = minHeight;
    invalidate();
  }

  unsigned Button::minHeight() const
  {
    return m_minHeight;
  }

  void Button::setController(Controller *controller)
  {
    m_textView->setController(controller);
    Element::setController(controller);
  }

  void Button::cacheButtonBody(EState state)
  {
    double const k = visualScale();
    m2::RectD const rr = GetBoundRect();

    graphics::Screen * cs = m_controller->GetCacheScreen();

    cs->beginFrame();

    unique_ptr<graphics::DisplayList> & dl = m_dls[state];

    dl.reset();
    dl.reset(cs->createDisplayList());

    cs->setDisplayList(dl.get());

    cs->drawRoundedRectangle(m2::RectD(-rr.SizeX() / 2,
                                       -rr.SizeY() / 2,
                                        rr.SizeX() / 2,
                                        rr.SizeY() / 2),
                             10 * k, color(state), depth());

    cs->setDisplayList(0);

    cs->endFrame();
  }

  void Button::cache()
  {
    cacheButtonBody(EActive);
    cacheButtonBody(EPressed);
  }

  void Button::purge()
  {
    m_dls.clear();
    m_textView->purge();
  }

  void Button::layout()
  {
    m_textView->setIsDirtyLayout(true);
  }

  m2::RectD Button::GetBoundRect() const
  {
    double const k = visualScale();

    m2::RectD tr(m_textView->GetBoundRect());
    m2::RectD rc(0, 0, tr.SizeX(), tr.SizeY());

    rc.Inflate(15 * k, 5 * k);

    double dx = 0;
    double dy = 0;

    if (rc.SizeX() < m_minWidth * k)
      dx = (m_minWidth * k - rc.SizeX()) / 2;
    if (rc.SizeY() < m_minHeight * k)
      dy = (m_minHeight * k - rc.SizeY()) / 2;

    rc.Inflate(dx, dy);
    rc.Offset(-rc.minX(), -rc.minY());

    m2::PointD pt = computeTopLeft(m2::PointD(rc.SizeX(), rc.SizeY()),
                                   pivot(),
                                   position());

    rc.Offset(pt);

    return rc;
  }

  void Button::draw(graphics::OverlayRenderer * r, math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      math::Matrix<double, 3, 3> const drawM = math::Shift(math::Identity<double, 3>(), pivot());

      r->drawDisplayList(m_dls.at(state()).get(), drawM * m);

      m_textView->draw(r, m);
    }
  }

  void Button::setPivot(m2::PointD const & pv)
  {
    m_textView->setPivot(pv);
    Element::setPivot(pv);
  }

  void Button::setFont(EState state, graphics::FontDesc const & font)
  {
    m_textView->setFont(state, font);
    Element::setFont(state, font);
  }

  void Button::setColor(EState state, graphics::Color const & c)
  {
    m_textView->setColor(state, c);
    Element::setColor(state, c);
  }
}
