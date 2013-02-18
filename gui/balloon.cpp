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
    : m_image(),
      m_textMarginLeft(10),
      m_textMarginTop(10),
      m_textMarginRight(10),
      m_textMarginBottom(10),
      m_imageMarginLeft(10),
      m_imageMarginTop(10),
      m_imageMarginRight(10),
      m_imageMarginBottom(10)
  {}

  Balloon::Balloon(Params const & p)
    : Element(p),
      m_boundRects(1)
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

    tp.m_text = p.m_text;
    tp.m_position = graphics::EPosRight;
    tp.m_pivot = m2::PointD(0, 0);
    tp.m_depth = depth() + 1;

    m_textView.reset(new TextView(tp));

    m_textView->setFont(Element::EActive, graphics::FontDesc(20, graphics::Color(255, 255, 255, 255)));

    ImageView::Params ip;

    ip.m_pivot = m2::PointD(0, 0);
    ip.m_position = graphics::EPosRight;
    ip.m_depth = depth() + 1;
    ip.m_image = p.m_image;

    m_imageView.reset(new ImageView(ip));

    m_arrowHeight = 10;
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
        r.setMaxY(r.maxY() + m_arrowHeight * k);
      else if (pos == graphics::EPosUnder)
        r.setMinY(r.minY() - m_arrowHeight * k);
      else if (pos == graphics::EPosRight)
        r.setMinX(r.minX() - m_arrowHeight * k);
      else if (pos == graphics::EPosLeft)
        r.setMaxX(r.maxX() + m_arrowHeight * k);

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

    double k = visualScale();
    double tml = m_textMarginLeft * k;
    double tmr = m_textMarginRight * k;
    double tmt = m_textMarginTop * k;
    double tmb = m_textMarginBottom * k;

    double iml = m_imageMarginLeft * k;
    double imr = m_imageMarginRight * k;
    double imt = m_imageMarginTop * k;
    double imb = m_imageMarginBottom * k;

    double arrowHeight = m_arrowHeight * k;

    double w = tml + tr.SizeX() + tmr
             + iml + ir.SizeX() + imr;

    double h = max(ir.SizeY() + imb + imt,
                   tr.SizeY() + tmb + tmt);

    m2::PointD const & pv = pivot();
    graphics::EPosition pos = position();

    if (pos == graphics::EPosAbove)
    {
      m_textView->setPivot(m2::PointD(pv.x - w / 2 + tml,
                                      pv.y - h / 2 - arrowHeight));
      m_imageView->setPivot(m2::PointD(pv.x + w / 2 - imr - ir.SizeX(),
                                       pv.y - h / 2 - arrowHeight));
    }
    else if (pos == graphics::EPosUnder)
    {
      m_textView->setPivot(m2::PointD(pv.x - w / 2 + tml,
                                      pv.y + h / 2 + arrowHeight));
      m_imageView->setPivot(m2::PointD(pv.x + w / 2 - imr - ir.SizeX(),
                                       pv.y + h / 2 + arrowHeight));
    }
    else if (pos == graphics::EPosLeft)
    {
      m_textView->setPivot(m2::PointD(pv.x - w - arrowHeight + tml,
                                      pv.y));
      m_imageView->setPivot(m2::PointD(pv.x - arrowHeight - imr - ir.SizeX(),
                                       pv.y));
    }
    else if (pos == graphics::EPosRight)
    {
      m_textView->setPivot(m2::PointD(pv.x + arrowHeight + tml,
                                      pv.y));
      m_imageView->setPivot(m2::PointD(pv.x + arrowHeight + tml + tr.SizeX() + tmr + imr,
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

    double k = visualScale();

    double bw = r.SizeX();
    double bh = r.SizeY();
    double aw = m_arrowWidth * k;
    double ah = m_arrowHeight * k;

    graphics::EPosition pos = position();
    graphics::Path<float> p;

    if (pos & graphics::EPosAbove)
    {
      bh -= ah;

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
      bh -= ah;

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

//      r->drawRectangle(roughBoundRect(), graphics::Color(0, 0, 255, 128), depth() + 1);
//      r->drawRectangle(m_textView->roughBoundRect(), graphics::Color(0, 255, 255, 128), depth() + 1);
//      r->drawRectangle(m_imageView->roughBoundRect(), graphics::Color(0, 255, 0, 128), depth() + 1);

      math::Matrix<double, 3, 3> drawM = math::Shift(
            math::Identity<double, 3>(),
            pivot() * m);

      r->drawDisplayList(m_displayList.get(), drawM);

      m_textView->draw(r, m);
      m_imageView->draw(r, m);
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

  void Balloon::setImage(graphics::Image::Info const & info)
  {
    m_imageView->setImage(info);
    setIsDirtyLayout(true);
    invalidate();
  }
}
