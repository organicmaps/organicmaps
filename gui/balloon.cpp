#include "balloon.hpp"
#include "controller.hpp"

#include "../geometry/transformations.hpp"

#include "../graphics/overlay_renderer.hpp"
#include "../graphics/brush.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/path.hpp"

#define TOP_SHADOW_MARGIN 2
#define BOTTOM_SHADOW_MARGIN 4
#define TEXT_LEFT_MARGIN 1
#define TEXT_RIGHT_MARGIN 25
#define ARROW_MARGIN 16

namespace gui
{
  Balloon::Params::Params()
    : m_image()
  {}

  Balloon::Balloon(Params const & p)
    : Element(p),
      m_balloonScale(1.0),
      m_textImageOffsetH(0.0),
      m_textImageOffsetV(0.0),
      m_boundRects(1)
  {
    TextView::Params tp;

    tp.m_position = graphics::EPosAboveRight;
    tp.m_pivot = m2::PointD(0, 0);
    tp.m_depth = depth() + 1;

    m_mainTextView.reset(new TextView(tp));
    m_mainTextView->setFont(Element::EActive, graphics::FontDesc(17, graphics::Color(0, 0, 0, 255)));

    TextView::Params auxTp;
    auxTp.m_text = p.m_auxText;
    auxTp.m_position = graphics::EPosAboveRight;
    auxTp.m_pivot = m2::PointD(0, 0);
    auxTp.m_depth = depth() + 1;

    m_auxTextView.reset(new TextView(auxTp));
    m_auxTextView->setFont(Element::EActive, graphics::FontDesc(12, graphics::Color(102, 102, 102, 255)));

    updateTextMode(p.m_mainText, p.m_auxText);

    ImageView::Params ip;

    ip.m_pivot = m2::PointD(0, 0);
    ip.m_position = graphics::EPosRight;
    ip.m_depth = depth() + 1;
    ip.m_image = p.m_image;

    m_imageView.reset(new ImageView(ip));
  }

  void Balloon::updateTextMode(string const & main, string const & aux)
  {
    m_textMode = NoText;
    if (!main.empty())
      m_textMode = (ETextMode)(m_textMode | SingleMainText);
    if (!aux.empty())
      m_textMode = (ETextMode)(m_textMode | SingleAuxText);
  }

  vector<m2::AnyRectD> const & Balloon::boundRects() const
  {
    if (isDirtyRect())
    {
      m2::RectD tr = m_mainTextView->roughBoundRect();
      m2::RectD auxTr = m_auxTextView->roughBoundRect();
      m2::RectD ir = m_imageView->roughBoundRect();

      double k = visualScale();

      tr.setMinX(tr.minX() - (m_borderLImg.m_size.x + TEXT_LEFT_MARGIN * k));
      tr.setMaxX(tr.maxX() + TEXT_RIGHT_MARGIN * k);

      auxTr.setMinX(auxTr.minX() - (m_borderLImg.m_size.x + TEXT_LEFT_MARGIN * k));
      auxTr.setMaxX(auxTr.maxX() + TEXT_RIGHT_MARGIN * k);

      ir.setMaxX(ir.maxX() + m_borderRImg.m_size.x);

      m2::RectD r(ir);
      if (m_textMode & SingleMainText)
        r.Add(tr);
      if (m_textMode & SingleAuxText)
        r.Add(auxTr);

      double diff = m_bodyImg.m_size.y + m_arrowImg.m_size.y - r.SizeY();
      double halfDiff = diff / 2.0;

      r.setMaxY(r.maxY() + halfDiff);
      r.setMinY(r.minY() - halfDiff);

      m_boundRects[0] = m2::AnyRectD(r);

      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  void Balloon::layout()
  {
    m_mainTextView->setIsDirtyLayout(true);
    m_auxTextView->setIsDirtyLayout(true);
    m_imageView->setIsDirtyLayout(true);

    m2::RectD mainTextRect = m_mainTextView->roughBoundRect();
    m2::RectD auxTextRect = m_auxTextView->roughBoundRect();
    m2::RectD imageRect = m_imageView->roughBoundRect();

    double k = visualScale();
    double leftMargin = m_borderLImg.m_size.x + TEXT_LEFT_MARGIN * k;
    double rightMargin = TEXT_RIGHT_MARGIN * k;

    double imageMargin = m_borderRImg.m_size.x;

    double mainWidth = leftMargin + mainTextRect.SizeX() + rightMargin;
    double auxWidth = leftMargin + auxTextRect.SizeX() + rightMargin;
    double imageWidth = imageRect.SizeX() + imageMargin;

    double maxWidth = max(mainWidth, auxWidth) + imageWidth;

    if (m_textMode & SingleMainText)
      layoutMainText(maxWidth, leftMargin);

    if (m_textMode & SingleAuxText)
      layoutAuxText(maxWidth, leftMargin);

    m2::PointD pv = pivot();

    graphics::EPosition pos = position();
    graphics::EPosition newPosition = graphics::EPosLeft;
    if (pos & graphics::EPosRight)
      pv.x = pv.x + m_arrowImg.m_size.x / 2.0 + ARROW_MARGIN * k + m_borderRImg.m_size.x - imageMargin;
    else if (pos & graphics::EPosLeft)
      pv.x = pv.x  + (maxWidth - m_borderRImg.m_size.x - ARROW_MARGIN * k - m_arrowImg.m_size.x / 2.0) - imageMargin;
    else
    {
      if (m_textMode != NoText)
        pv.x = pv.x + maxWidth / 2.0 - imageMargin;
      else
        newPosition = graphics::EPosCenter;
    }
    pv.y -= (m_arrowImg.m_size.y + (m_bodyImg.m_size.y - (TOP_SHADOW_MARGIN + BOTTOM_SHADOW_MARGIN) * k) / 2.0);

    m_imageView->setPivot(pv);

    m_imageView->setPosition(newPosition);
  }

  void Balloon::layoutMainText(double balloonWidth,
                               double leftMargin)
  {
    m2::PointD pv = pivot();
    graphics::EPosition newPosition = graphics::EPosRight;

    graphics::EPosition balloonPos = position();
    double k = visualScale();
    if (balloonPos & graphics::EPosRight)
      pv.x = pv.x - (balloonWidth - m_borderRImg.m_size.x - ARROW_MARGIN * k - m_arrowImg.m_size.x / 2.0) + leftMargin;
    else if (balloonPos & graphics::EPosLeft)
      pv.x = pv.x - (m_borderLImg.m_size.x + ARROW_MARGIN * k + m_arrowImg.m_size.x / 2.0) + leftMargin;
    else
      pv.x = pv.x - balloonWidth / 2.0 + leftMargin;

    double heightWithoutShadows = m_bodyImg.m_size.y - (BOTTOM_SHADOW_MARGIN + TOP_SHADOW_MARGIN) * visualScale();
    if (m_textMode == DualText)
    {
      pv.y = pv.y - (heightWithoutShadows / 2.0 + m_arrowImg.m_size.y);
      newPosition = graphics::EPosAboveRight;
    }
    else
      pv.y = pv.y - (heightWithoutShadows / 2.0 + m_arrowImg.m_size.y);

    m_mainTextView->setPivot(pv);
    m_mainTextView->setPosition(newPosition);
  }

  void Balloon::layoutAuxText(double balloonWidth,
                              double leftMargin)
  {
    m2::PointD pv = pivot();
    graphics::EPosition newPosition = graphics::EPosRight;

    double k = visualScale();
    graphics::EPosition balloonPos = position();
    if (balloonPos & graphics::EPosRight)
      pv.x = pv.x - (balloonWidth - m_borderRImg.m_size.x - ARROW_MARGIN * k - m_arrowImg.m_size.x / 2.0) + leftMargin;
    else if (balloonPos & graphics::EPosLeft)
      pv.x = pv.x - (m_borderLImg.m_size.x + ARROW_MARGIN * visualScale() + m_arrowImg.m_size.x / 2.0) + leftMargin;
    else
      pv.x = pv.x - balloonWidth / 2.0 + leftMargin;

    double heightWithoutShadows = m_bodyImg.m_size.y - (BOTTOM_SHADOW_MARGIN + TOP_SHADOW_MARGIN) * visualScale();
    if (m_textMode == DualText)
    {
      pv.y = pv.y - (heightWithoutShadows  / 6.0 + m_arrowImg.m_size.y);
      newPosition = graphics::EPosAboveRight;
    }
    else
      pv.y = pv.y - (heightWithoutShadows / 2.0 + m_arrowImg.m_size.y);

    m_auxTextView->setPivot(pv);
    m_auxTextView->setPosition(newPosition);
  }

  void Balloon::setPivot(m2::PointD const & pv)
  {
    Element::setPivot(pv);
    layout();
  }

  void Balloon::cacheLeftBorder(graphics::Screen * cs,
                                double offsetX)
  {
    uint32_t borderID = cs->mapInfo(m_borderLImg);

    double offsetY = m_borderLImg.m_size.y + m_arrowImg.m_size.y - BOTTOM_SHADOW_MARGIN * visualScale();
    math::Matrix<double, 3, 3> m = math::Shift(
                                              math::Identity<double, 3>(),
                                               -offsetX, -offsetY);

    cs->drawImage(m, borderID, depth());
  }

  void Balloon::cacheRightBorder(graphics::Screen * cs,
                                 double offsetX)
  {
    uint32_t borderID = cs->mapInfo(m_borderRImg);

    double offsetY = m_borderRImg.m_size.y + m_arrowImg.m_size.y - BOTTOM_SHADOW_MARGIN * visualScale();
    math::Matrix<double, 3, 3> m = math::Shift(
                                               math::Identity<double, 3>(),
                                               offsetX - m_borderRImg.m_size.x, -offsetY);

    cs->drawImage(m, borderID, depth());
  }

  void Balloon::cacheBody(graphics::Screen * cs,
                          double offsetX,
                          double bodyWidth)
  {
    uint32_t bodyID = cs->mapInfo(m_bodyImg);
    int32_t bodyCount = (bodyWidth + m_bodyImg.m_size.x - 1) / (double)(m_bodyImg.m_size.x );
    double offsetY = m_bodyImg.m_size.y + m_arrowImg.m_size.y - BOTTOM_SHADOW_MARGIN * visualScale();
    offsetY = -offsetY;
    double currentOffsetX = -offsetX;

    math::Matrix<double, 3, 3> m = math::Shift(math::Identity<double, 3>(),
                                               currentOffsetX, offsetY);

    for (int32_t i = 0; i < bodyCount - 1; ++i)
    {
      cs->drawImage(m, bodyID, depth());

      m = math::Shift(m, m2::PointF(m_bodyImg.m_size.x, 0));
      currentOffsetX += m_bodyImg.m_size.x;
    }

    double lastTileWidth = bodyWidth - (m_bodyImg.m_size.x) * (bodyCount - 1);
    double scaleFactor = (double(lastTileWidth))/ (double)(m_bodyImg.m_size.x);

    math::Matrix<double, 3, 3> lastBodyM = math::Shift(
                                                      math::Scale(math::Identity<double, 3>(), scaleFactor, 1.0),
                                                        currentOffsetX, offsetY);
    cs->drawImage(lastBodyM, bodyID, depth());
  }

  void Balloon::cache()
  {
    m2::RectD const & r = roughBoundRect();

    double bw = r.SizeX();

    graphics::Screen * cs = m_controller->GetCacheScreen();

    m_displayList.reset();
    m_displayList.reset(cs->createDisplayList());

    cs->beginFrame();
    cs->setDisplayList(m_displayList.get());

    uint32_t arrowID = cs->mapInfo(m_arrowImg);
    math::Matrix<double, 3, 3> arrowM = math::Shift(math::Identity<double, 3>(),
                                                    -(m_arrowImg.m_size.x / 2.0), -((double)m_arrowImg.m_size.y + 1));
    cs->drawImage(arrowM, arrowID, depth() + 1);

    double leftBorderOffset = 0.0;
    double bodyWidth = bw - (m_borderLImg.m_size.x + m_borderRImg.m_size.x);
    double rightBorderOffset = 0.0;

    graphics::EPosition pos = position();
    if (pos & graphics::EPosRight)
    {
      rightBorderOffset = m_borderLImg.m_size.x + ARROW_MARGIN * visualScale() + m_arrowImg.m_size.x / 2.0;
      leftBorderOffset = bw - rightBorderOffset;
    }
    else if (pos & graphics::EPosLeft)
    {
      leftBorderOffset = m_borderRImg.m_size.x + ARROW_MARGIN * visualScale() + m_arrowImg.m_size.x / 2.0;
      rightBorderOffset = bw - leftBorderOffset;
    }
    else
      rightBorderOffset = leftBorderOffset = bw / 2.0;

    cacheLeftBorder(cs, leftBorderOffset);
    cacheRightBorder(cs, rightBorderOffset);
    cacheBody(cs, leftBorderOffset - m_borderLImg.m_size.x, bodyWidth);

    cs->setDisplayList(0);
    cs->endFrame();
  }

  void Balloon::purge()
  {
    m_mainTextView->purge();
    m_auxTextView->purge();
    m_imageView->purge();
    m_displayList.reset();
  }

  void Balloon::initBgImages()
  {
    graphics::EDensity density = m_controller->GetDensity();
    m_borderLImg = graphics::Image::Info("round-left.png", density /*, graphics::Image::Info::DitherIt()*/);
    m_borderRImg = graphics::Image::Info("round-right.png", density /*, graphics::Image::Info::DitherIt()*/);
    m_bodyImg = graphics::Image::Info("mid.png", density /*, graphics::Image::Info::DitherIt()*/);
    m_arrowImg = graphics::Image::Info("pin.png", density /*, graphics::Image::Info::DitherIt()*/);
  }

  void Balloon::setController(Controller * controller)
  {
    Element::setController(controller);
    m_mainTextView->setController(controller);
    m_auxTextView->setController(controller);
    m_imageView->setController(controller);

    initBgImages();
  }

  void Balloon::draw(graphics::OverlayRenderer * r,
                     math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      math::Matrix<double, 3, 3> drawM = math::Shift(
            math::Identity<double, 3>(),
            pivot() * m);

      math::Matrix<double, 3, 3> scaleM = math::Scale(math::Identity<double, 3>(),
                                                      m_balloonScale, m_balloonScale);

      r->drawDisplayList(m_displayList.get(), scaleM * drawM);

      math::Matrix<double, 3, 3> offsetM = math::Shift(math::Identity<double, 3>(),
                                                       m_textImageOffsetV, m_textImageOffsetH);

      if (m_textMode | SingleMainText)
        m_mainTextView->draw(r, offsetM * m);
      if (m_textMode | SingleAuxText)
        m_auxTextView->draw(r, offsetM * m);
      m_imageView->draw(r, offsetM * m);
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

  void Balloon::setText(string const & main, string const & aux)
  {
    updateTextMode(main, aux);
    m_mainTextView->setText(main);
    m_auxTextView->setText(aux);
    setIsDirtyLayout(true);
    calcMaxTextWidth();
    invalidate();
  }

  void Balloon::setImage(graphics::Image::Info const & info)
  {
    m_imageView->setImage(info);
    setIsDirtyLayout(true);
    invalidate();
  }

  void Balloon::onScreenSize(int w, int h)
  {
    int minEdge = min(w, h);
    m_maxWidth = minEdge - 64;
    setIsDirtyLayout(true);
    calcMaxTextWidth();
    invalidate();
  }

  void Balloon::calcMaxTextWidth()
  {
    double k = visualScale();
    double textMargin = m_borderLImg.m_size.x + TEXT_LEFT_MARGIN * k +
                        TEXT_RIGHT_MARGIN * k;

    double imageWidth = m_borderRImg.m_size.x;
    unsigned maxTextWidth = ceil(m_maxWidth - (textMargin + imageWidth));
    m_mainTextView->setMaxWidth(maxTextWidth);
    m_auxTextView->setMaxWidth(maxTextWidth);
  }
}
