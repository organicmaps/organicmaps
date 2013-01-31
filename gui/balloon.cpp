#include "balloon.hpp"
#include "controller.hpp"

#include "../geometry/transformations.hpp"

#include "../graphics/overlay_renderer.hpp"
#include "../graphics/brush.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/path.hpp"

namespace gui
{
  Balloon::Params::Params()
  {}

  Balloon::Balloon(Params const & p)
    : Element(p),
      m_boundRects(1),
      m_text(p.m_text),
      m_image(p.m_image)
  {
    m_textMarginLeft = p.m_textMarginLeft;
    m_textMarginRight = p.m_textMarginRight;
    m_textMarginTop = p.m_textMarginTop;
    m_textMarginBottom = p.m_textMarginBottom;

    m_imageMarginLeft = p.m_imageMarginLeft;
    m_imageMarginRight = p.m_imageMarginRight;
    m_imageMarginTop = p.m_imageMarginTop;
    m_imageMarginBottom = p.m_imageMarginBottom;

    TextView::Params tp;

    tp.m_text = m_text;
    tp.m_position = graphics::EPosRight;
    tp.m_pivot = m2::PointD(0, 0);
    tp.m_depth = depth();

    m_textView.reset(new TextView(tp));

    m_textView->setFont(Element::EActive, graphics::FontDesc(20, graphics::Color(0, 0, 0, 255), true));

    ImageView::Params ip;

    ip.m_pivot = m2::PointD(0, 0);
    ip.m_position = graphics::EPosRight;
    ip.m_depth = depth();
    ip.m_image = m_image;

    m_imageView.reset(new ImageView(ip));

    m_arrowHeight = 20;
    m_arrowAngle = ang::DegreeToRad(90);
    m_arrowWidth = 2 * tan(m_arrowAngle / 2) * m_arrowHeight;
  }

  vector<m2::AnyRectD> const & Balloon::boundRects() const
  {
    if (isDirtyRect())
    {
      m2::RectD tr = m_textView->roughBoundRect();
      m2::RectD ir = m_imageView->roughBoundRect();

      double k = visualScale();

      tr.setMinX(tr.minX() - m_textMarginLeft * k);
      tr.setMinY(tr.minY() - m_textMarginTop * k);
      tr.setMaxX(tr.maxX() + m_textMarginRight * k);
      tr.setMaxY(tr.maxY() + m_textMarginBottom * k);

      ir.setMinX(ir.minX() - m_imageMarginLeft * k);
      ir.setMinY(ir.minY() - m_imageMarginTop * k);
      ir.setMaxX(ir.maxX() + m_imageMarginRight * k);
      ir.setMaxY(ir.maxY() + m_imageMarginBottom * k);

      m2::RectD r(tr);
      r.Add(ir);

      graphics::EPosition pos = position();

      if (pos == graphics::EPosAbove)
        r.setMaxY(r.maxY() + m_arrowHeight);
      else if (pos == graphics::EPosUnder)
        r.setMinY(r.minY() - m_arrowHeight);
      else if (pos == graphics::EPosRight)
        r.setMinX(r.minX() - m_arrowHeight);
      else if (pos == graphics::EPosLeft)
        r.setMaxX(r.maxX() + m_arrowHeight);

      m_boundRects[0] = m2::AnyRectD(r);

      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  void Balloon::layout()
  {
    m_textView->setIsDirtyLayout(true);
    m_imageView->setIsDirtyLayout(true);

    m2::RectD tr = m_textView->roughBoundRect();
    m2::RectD ir = m_imageView->roughBoundRect();

    double w = m_textMarginLeft + tr.SizeX() + m_textMarginRight
             + m_imageMarginLeft + ir.SizeX() + m_imageMarginRight;

    double h = max(ir.SizeY() + m_imageMarginBottom + m_imageMarginTop,
                   tr.SizeY() + m_textMarginBottom + m_textMarginTop);

    m2::PointD const & pv = pivot();
    graphics::EPosition pos = position();

    if (pos == graphics::EPosAbove)
    {
      m_textView->setPivot(m2::PointD(pv.x - w / 2 + m_textMarginLeft,
                                      pv.y - h / 2 - m_arrowHeight));
      m_imageView->setPivot(m2::PointD(pv.x + w / 2 - m_imageMarginRight - ir.SizeX(),
                                       pv.y - h / 2 - m_arrowHeight));
    }
    else if (pos == graphics::EPosUnder)
    {
      m_textView->setPivot(m2::PointD(pv.x - w / 2 + m_textMarginLeft,
                                      pv.y + h / 2 + m_arrowHeight));
      m_imageView->setPivot(m2::PointD(pv.x + w / 2 - m_imageMarginRight - ir.SizeX(),
                                       pv.y + h / 2 + m_arrowHeight));
    }
    else if (pos == graphics::EPosLeft)
    {
      m_textView->setPivot(m2::PointD(pv.x - w - m_arrowHeight + m_textMarginLeft,
                                      pv.y));
      m_imageView->setPivot(m2::PointD(pv.x - m_arrowHeight - m_imageMarginRight - ir.SizeX(),
                                       pv.y));
    }
    else if (pos == graphics::EPosRight)
    {
      m_textView->setPivot(m2::PointD(pv.x + m_arrowHeight + m_textMarginLeft,
                                      pv.y));
      m_imageView->setPivot(m2::PointD(pv.x + m_arrowHeight + m_textMarginLeft + tr.SizeX() + m_textMarginRight + m_imageMarginRight,
                                       pv.y));
    }

    m_textView->setPosition(graphics::EPosRight);
    m_imageView->setPosition(graphics::EPosRight);
  }

  void Balloon::setPivot(m2::PointD const & pv)
  {
    Element::setPivot(pv);
    layout();
  }

  void Balloon::cache()
  {
    graphics::Screen * cs = m_controller->GetCacheScreen();

    m_displayList.reset();
    m_displayList.reset(cs->createDisplayList());

    cs->beginFrame();
    cs->setDisplayList(m_displayList.get());

    m2::RectD const & r = roughBoundRect();

    double bw = r.SizeX();
    double bh = r.SizeY();
    double aw = m_arrowWidth;
    double ah = m_arrowHeight;

    graphics::EPosition pos = position();
    graphics::Path<float> p;

    if (pos & graphics::EPosAbove)
    {
      bh -= m_arrowHeight;

      p.reset(m2::PointF(-aw / 2, -ah));
      p.lineRel(m2::PointF(-bw / 2 + aw / 2, 0));
      p.lineRel(m2::PointF(0, -bh));
      p.lineRel(m2::PointF(bw, 0));
      p.lineRel(m2::PointF(0, bh));
      p.lineRel(m2::PointF(-bw / 2 + aw / 2, 0));
      p.lineRel(m2::PointF(-aw / 2, ah));
      p.lineRel(m2::PointF(-aw / 2, -ah));
    }
    else if (pos & graphics::EPosUnder)
    {
      bh -= m_arrowHeight;

      p.reset(m2::PointF(aw / 2, ah));
      p.lineRel(m2::PointF(bw / 2 - aw / 2, 0));
      p.lineRel(m2::PointF(0, bh));
      p.lineRel(m2::PointF(-bw, 0));
      p.lineRel(m2::PointF(0, -bh));
      p.lineRel(m2::PointF(bw / 2 - aw / 2, 0));
      p.lineRel(m2::PointF(aw / 2, -ah));
      p.lineRel(m2::PointF(aw / 2, ah));
    }

    graphics::Color c(0, 0, 0, 128);

    uint32_t colorID = cs->mapInfo(graphics::Brush::Info(c));

    cs->drawTrianglesFan(p.points(), p.size(), colorID, depth());

    cs->setDisplayList(0);
    cs->endFrame();
  }

  void Balloon::purge()
  {
    m_textView->purge();
    m_imageView->purge();
    m_displayList.reset();
  }

  void Balloon::setController(Controller * controller)
  {
    Element::setController(controller);
    m_textView->setController(controller);
    m_imageView->setController(controller);
  }

  void Balloon::draw(graphics::OverlayRenderer * r,
                     math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

//      r->drawRectangle(roughBoundRect(), graphics::Color(0, 0, 255, 128), depth());
//      r->drawRectangle(m_textView->roughBoundRect(), graphics::Color(0, 255, 255, 128), depth());
//      r->drawRectangle(m_imageView->roughBoundRect(), graphics::Color(0, 255, 0, 128), depth());

      math::Matrix<double, 3, 3> drawM = math::Shift(
            math::Identity<double, 3>(),
            pivot() * m);

      r->drawDisplayList(m_displayList.get(), drawM);

      m_textView->draw(r, m);
      m_imageView->draw(r, m);

      m2::RectD r1(pivot() * m, pivot() * m);
      r1.Inflate(2, 2);

      r->drawRectangle(r1, graphics::Color(255, 0, 0, 255), graphics::maxDepth);
    }
  }

  void Balloon::setOnClickListener(TOnClickListener const & fn)
  {
    m_onClickListener = fn;
  }

  bool Balloon::onTapStarted(m2::PointD const & pt)
  {
    setState(EPressed);
    invalidate();
    return false;
  }

  bool Balloon::onTapCancelled(m2::PointD const & pt)
  {
    setState(EActive);
    invalidate();
    return false;
  }

  bool Balloon::onTapEnded(m2::PointD const & pt)
  {
    setState(EActive);
    if (m_onClickListener)
      m_onClickListener(this);
    invalidate();
    return false;
  }

  bool Balloon::onTapMoved(m2::PointD const & pt)
  {
    invalidate();
    return false;
  }

  void Balloon::setText(string const & s)
  {
    m_textView->setText(s);
    setIsDirtyLayout(true);
    invalidate();
  }
}
