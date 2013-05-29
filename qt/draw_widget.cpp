#include "draw_widget.hpp"
#include "proxystyle.hpp"
#include "slider_ctrl.hpp"

#include "../map/render_policy.hpp"

#include "../search/result.hpp"

#include "../gui/controller.hpp"

#include "../graphics/opengl/opengl.hpp"

#include "../platform/settings.hpp"
#include "../platform/platform.hpp"

#include <QtCore/QLocale>

#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>


namespace qt
{
  const unsigned LONG_TOUCH_MS = 1000;

  QtVideoTimer::QtVideoTimer(TFrameFn frameFn)
    : ::VideoTimer(frameFn)
  {}

  void QtVideoTimer::start()
  {
    m_timer = new QTimer();
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(TimerElapsed()));
    resume();
  }

  void QtVideoTimer::pause()
  {
    m_timer->stop();
    m_state = EPaused;
  }

  void QtVideoTimer::resume()
  {
    m_timer->start(1000 / 60);
    m_state = ERunning;
  }

  void QtVideoTimer::stop()
  {
    pause();
    delete m_timer;
    m_timer = 0;
    m_state = EStopped;
  }

  void QtVideoTimer::TimerElapsed()
  {
    m_frameFn();
  }

  DrawWidget::DrawWidget(QWidget * pParent)
    : QGLWidget(pParent),
      m_isInitialized(false),
      m_isTimerStarted(false),
      m_framework(new Framework()),
      m_isDrag(false),
      m_isRotate(false),
      //m_redrawInterval(100),
      m_pScale(0)
  {
    //m_timer = new QTimer(this);
    //connect(m_timer, SIGNAL(timeout()), this, SLOT(ScaleTimerElapsed()));
  }

  DrawWidget::~DrawWidget()
  {
    m_framework.reset();
  }

  void DrawWidget::PrepareShutdown()
  {
    KillPressTask();
    ASSERT(isValid(), ());
    makeCurrent();
    m_framework->PrepareToShutdown();
    m_videoTimer.reset();
  }

  void DrawWidget::SetScaleControl(QScaleSlider * pScale)
  {
    m_pScale = pScale;

    connect(m_pScale, SIGNAL(actionTriggered(int)), this, SLOT(ScaleChanged(int)));
  }

  void DrawWidget::UpdateNow()
  {
    update();
  }

  void DrawWidget::UpdateAfterSettingsChanged()
  {
    m_framework->SetupMeasurementSystem();
  }

  bool DrawWidget::LoadState()
  {
    pair<int, int> widthAndHeight;
    if (!Settings::Get("DrawWidgetSize", widthAndHeight))
      return false;

   m_framework->OnSize(widthAndHeight.first, widthAndHeight.second);

    if (!m_framework->LoadState())
      return false;

    m_framework->LoadBookmarks();

    UpdateScaleControl();
    return true;
  }

  void DrawWidget::SaveState()
  {
    pair<int, int> widthAndHeight(width(), height());
    Settings::Set("DrawWidgetSize", widthAndHeight);

    m_framework->SaveState();
  }

  void DrawWidget::MoveLeft()
  {
    m_framework->Move(math::pi, 0.5);
    emit ViewportChanged();
  }

  void DrawWidget::MoveRight()
  {
    m_framework->Move(0.0, 0.5);
    emit ViewportChanged();
  }

  void DrawWidget::MoveUp()
  {
    m_framework->Move(math::pi/2.0, 0.5);
    emit ViewportChanged();
  }

  void DrawWidget::MoveDown()
  {
    m_framework->Move(-math::pi/2.0, 0.5);
    emit ViewportChanged();
  }

  void DrawWidget::ScalePlus()
  {
    m_framework->Scale(2.0);
    UpdateScaleControl();
    emit ViewportChanged();
  }

  void DrawWidget::ScaleMinus()
  {
    m_framework->Scale(0.5);
    UpdateScaleControl();
    emit ViewportChanged();
  }

  void DrawWidget::ScalePlusLight()
  {
    m_framework->Scale(1.5);
    UpdateScaleControl();
    emit ViewportChanged();
  }

  void DrawWidget::ScaleMinusLight()
  {
    m_framework->Scale(2.0/3.0);
    UpdateScaleControl();
    emit ViewportChanged();
  }

  void DrawWidget::ShowAll()
  {
    m_framework->ShowAll();
    UpdateScaleControl();
    emit ViewportChanged();
  }

  void DrawWidget::Repaint()
  {
    m_framework->Invalidate();
  }

  VideoTimer * DrawWidget::CreateVideoTimer()
  {
//#ifdef OMIM_OS_MAC
//    return CreateAppleVideoTimer(bind(&DrawWidget::DrawFrame, this));
//#else
    /// Using timer, which doesn't use the separate thread
    /// for performing an action. This avoids race conditions in Framework.
    /// see issue #717
    return new QtVideoTimer(bind(&DrawWidget::DrawFrame, this));
//#endif
  }

  void DrawWidget::ScaleChanged(int action)
  {
    if (action != QAbstractSlider::SliderNoAction)
    {
      double const factor = m_pScale->GetScaleFactor();
      if (factor != 1.0)
      {
        m_framework->Scale(factor);
        emit ViewportChanged();
      }
    }
  }

  void DrawWidget::initializeGL()
  {
    // we'll perform swap by ourselves, see issue #333
    setAutoBufferSwap(false);

    if (!m_isInitialized)
    {
      m_videoTimer.reset(CreateVideoTimer());

      shared_ptr<qt::gl::RenderContext> primaryRC(new qt::gl::RenderContext(this));

      graphics::ResourceManager::Params rmParams;
      rmParams.m_rtFormat = graphics::Data8Bpp;
      rmParams.m_texFormat = graphics::Data8Bpp;
      rmParams.m_texRtFormat = graphics::Data4Bpp;
      rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

      RenderPolicy::Params rpParams;

      rpParams.m_screenWidth = QApplication::desktop()->geometry().width();
      rpParams.m_screenHeight = QApplication::desktop()->geometry().height();

      rpParams.m_videoTimer = m_videoTimer.get();
      rpParams.m_useDefaultFB = true;
      rpParams.m_rmParams = rmParams;
      rpParams.m_primaryRC = primaryRC;
      rpParams.m_skinName = "basic.skn";

      if (QApplication::desktop()->physicalDpiX() < 180)
        rpParams.m_density = graphics::EDensityMDPI;
      else
        rpParams.m_density = graphics::EDensityXHDPI;

      try
      {
        m_framework->SetRenderPolicy(CreateRenderPolicy(rpParams));
      }
      catch (graphics::gl::platform_unsupported const & e)
      {
        LOG(LERROR, ("OpenGL platform is unsupported, reason: ", e.what()));
        /// @todo Show "Please Update Drivers" dialog and close the program.
      }
      catch (RootException const & e)
      {
        LOG(LERROR, (e.what()));
      }

      graphics::EDensity const density = m_framework->GetRenderPolicy()->Density();
      m_images[IMAGE_PLUS] = ImageT("plus.png", density);
      m_images[IMAGE_ARROW] = ImageT("right-arrow.png", density);
      m_isInitialized = true;
    }
  }

  void DrawWidget::resizeGL(int w, int h)
  {
    m_framework->OnSize(w, h);
    m_framework->Invalidate();

    if (m_isInitialized && m_isTimerStarted)
      DrawFrame();

    UpdateScaleControl();
    emit ViewportChanged();
  }

  void DrawWidget::paintGL()
  {
    if (m_isInitialized && !m_isTimerStarted)
    {
      // timer should be started upon the first repaint request to fully initialized GLWidget.
      m_isTimerStarted = true;
      (void)m_framework->SetUpdatesEnabled(true);
    }

    m_framework->Invalidate();
  }

  void DrawWidget::DrawFrame()
  {
    if (m_framework->NeedRedraw())
    {
      makeCurrent();
      m_framework->SetNeedRedraw(false);

      shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_framework->GetRenderPolicy()->GetDrawer().get()));

      m_framework->BeginPaint(paintEvent);
      m_framework->DoPaint(paintEvent);

      // swapping buffers before ending the frame, see issue #333
      swapBuffers();

      m_framework->EndPaint(paintEvent);
      doneCurrent();
    }
  }

  void DrawWidget::StartPressTask(double x, double y, unsigned ms)
  {
    m_taskX = x;
    m_taskY = y;
#ifndef OMIM_PRODUCTION
    m_scheduledTasks.reset(new ScheduledTask(bind(&DrawWidget::OnPressTaskEvent, this, x, y, ms), ms));
#endif
  }

  void DrawWidget::KillPressTask()
  {
    if (m_scheduledTasks)
    {
      m_scheduledTasks->Cancel();
      m_scheduledTasks.reset();
    }
  }

  void DrawWidget::OnPressTaskEvent(double x, double y, unsigned ms)
  {
    m2::PointD pt(x, y);
    m2::PointD pivot;

    Framework::AddressInfo addressInfo;
    BookmarkAndCategory bm;
    switch (m_framework->GetBookmarkOrPoi(pt, pivot, addressInfo, bm))
    {
    case Framework::BOOKMARK:
      {
        const Bookmark * bookmark = m_framework->GetBmCategory(bm.first)->GetBookmark(bm.second);
        ActivatePopup(bookmark->GetOrg(), bookmark->GetName(), IMAGE_ARROW);
        return;
      }

    case Framework::POI:
      if (!GetBookmarkBalloon()->isVisible())
      {
        ActivatePopupWithAdressInfo(m_framework->PtoG(pivot), addressInfo);
        return;
      }
      break;

    default:
      if (ms == LONG_TOUCH_MS)
      {
        m_framework->GetAddressInfo(pt, addressInfo);
        ActivatePopupWithAdressInfo(m_framework->PtoG(pt), addressInfo);
        return;
      }
    }

    DiactivatePopup();
  }

  void DrawWidget::ActivatePopup(m2::PointD const & pivot, string const & name, PopupImageIndexT index)
  {
    BookmarkBalloon * balloon = GetBookmarkBalloon();

    m_framework->DisablePlacemark();

    balloon->setImage(m_images[index]);
    balloon->setGlbPivot(pivot);
    balloon->setBookmarkName(name);
    balloon->setIsVisible(true);

    m_framework->Invalidate();
  }

  void DrawWidget::ActivatePopupWithAdressInfo(m2::PointD const & pivot, Framework::AddressInfo const & addrInfo)
  {
    string name = addrInfo.FormatPinText();
    if (name.empty())
      name = m_framework->GetStringsBundle().GetString("dropped_pin");

    ActivatePopup(pivot, name, IMAGE_PLUS);

    m_framework->DrawPlacemark(pivot);
    m_framework->Invalidate();
  }

  void DrawWidget::DiactivatePopup()
  {
    BookmarkBalloon * balloon = GetBookmarkBalloon();

    balloon->setIsVisible(false);
    m_framework->DisablePlacemark();
    m_framework->Invalidate();
  }

  void DrawWidget::CreateBookmarkBalloon()
  {
    BookmarkBalloon::Params bp;

    bp.m_position = graphics::EPosAbove;
    bp.m_depth = graphics::maxDepth;
    bp.m_pivot = m2::PointD(0.0, 0.0);
    bp.m_text = "Bookmark";
    bp.m_textMarginLeft = 10;
    bp.m_textMarginTop = 7;
    bp.m_textMarginRight = 10;
    bp.m_textMarginBottom = 10;
    bp.m_imageMarginLeft = 0;
    bp.m_imageMarginTop = 7;
    bp.m_imageMarginRight = 10;
    bp.m_imageMarginBottom = 10;
    bp.m_framework = m_framework.get();

    m_bookmarkBalloon.reset(new BookmarkBalloon(bp));
    m_bookmarkBalloon->setIsVisible(false);

    /// @todo Process adding bookmark.
    //m_bookmarkBalloon->setOnClickListener(bind(&DrawWidget::OnBalloonClick, this, _1));

    m_framework->GetGuiController()->AddElement(m_bookmarkBalloon);
  }

  BookmarkBalloon * DrawWidget::GetBookmarkBalloon()
  {
    if (!m_bookmarkBalloon)
      CreateBookmarkBalloon();

    return m_bookmarkBalloon.get();
  }

  namespace
  {
    DragEvent get_drag_event(QMouseEvent * e)
    {
      QPoint const p = e->pos();
      return DragEvent(p.x(), p.y());
    }

    RotateEvent get_rotate_event(QPoint const & pt, QPoint const & centerPt)
    {
      return RotateEvent(centerPt.x(), centerPt.y(), pt.x(), pt.y());
    }

    void add_string(QMenu & menu, string const & s)
    {
      (void)menu.addAction(QString::fromUtf8(s.c_str()));
    }
  }

  void DrawWidget::mousePressEvent(QMouseEvent * e)
  {
    QGLWidget::mousePressEvent(e);

    KillPressTask();

    if (e->button() == Qt::LeftButton)
    {
      if (m_framework->GetGuiController()->OnTapStarted(m2::PointU(e->pos().x(), e->pos().y())))
        return;

      if (e->modifiers() & Qt::ControlModifier)
      {
        // starting rotation
        m_framework->StartRotate(get_rotate_event(e->pos(), this->rect().center()));

        setCursor(Qt::CrossCursor);
        m_isRotate = true;
      }
      else
      {
        StartPressTask(e->x(), e->y(), LONG_TOUCH_MS);

        // starting drag
        m_framework->StartDrag(get_drag_event(e));

        setCursor(Qt::CrossCursor);
        m_isDrag = true;
      }
    }
    else if (e->button() == Qt::RightButton)
    {
      // show feature types
      QPoint const & qp = e->pos();
      m2::PointD const pt(qp.x(), qp.y());
      QMenu menu;

      // Get POI under cursor or nearest address by point.
      Framework::AddressInfo info;
      m2::PointD dummy;
      if (m_framework->GetVisiblePOI(pt, dummy, info))
      {
        add_string(menu, "POI");
      }
      else
      {
        m_framework->GetAddressInfo(pt, info);
      }

      // Get feature types under cursor.
      vector<string> types;
      m_framework->GetFeatureTypes(pt, types);
      for (size_t i = 0; i < types.size(); ++i)
        add_string(menu, types[i]);

      (void)menu.addSeparator();

      // Format address and types.
      if (!info.m_name.empty())
        add_string(menu, info.m_name);
      add_string(menu, info.FormatAddress());
      add_string(menu, info.FormatTypes());

      menu.exec(qp);
    }
  }

  void DrawWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    QGLWidget::mouseDoubleClickEvent(e);

    KillPressTask();

    if (e->button() == Qt::LeftButton)
    {
      StopDragging(e);

      m_framework->ScaleToPoint(ScaleToPointEvent(e->pos().x(), e->pos().y(), 1.5));

      UpdateScaleControl();
      emit ViewportChanged();
    }
  }

  void DrawWidget::mouseMoveEvent(QMouseEvent * e)
  {
    QGLWidget::mouseMoveEvent(e);

    m2::PointD const pt(e->pos().x(), e->pos().y());

    double const minDist = m_framework->GetVisualScale() * 10.0;
    if (fabs(pt.x - m_taskX) > minDist || fabs(pt.y - m_taskY) > minDist)
      KillPressTask();

    m_framework->GetGuiController()->OnTapMoved(pt);

    if (m_isDrag)
      m_framework->DoDrag(get_drag_event(e));

    if (m_isRotate)
      m_framework->DoRotate(get_rotate_event(e->pos(), rect().center()));
  }

  void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    QGLWidget::mouseReleaseEvent(e);

    m_framework->GetGuiController()->OnTapEnded(m2::PointU(e->pos().x(), e->pos().y()));

    StopDragging(e);
    StopRotating(e);

    emit ViewportChanged();
  }

  void DrawWidget::keyReleaseEvent(QKeyEvent * e)
  {
    QGLWidget::keyReleaseEvent(e);

    StopRotating(e);

    emit ViewportChanged();
  }

  void DrawWidget::StopRotating(QMouseEvent * e)
  {
    if (m_isRotate && (e->button() == Qt::LeftButton))
    {
      m_framework->StopRotate(get_rotate_event(e->pos(), this->rect().center()));

      setCursor(Qt::ArrowCursor);
      m_isRotate = false;
    }
  }

  void DrawWidget::StopRotating(QKeyEvent * e)
  {
    if (m_isRotate && (e->key() == Qt::Key_Control))
    {
      m_framework->StopRotate(get_rotate_event(this->mapFromGlobal(QCursor::pos()), this->rect().center()));
    }
  }

  void DrawWidget::StopDragging(QMouseEvent * e)
  {
    if (m_isDrag && e->button() == Qt::LeftButton)
    {
      m_framework->StopDrag(get_drag_event(e));

      setCursor(Qt::ArrowCursor);
      m_isDrag = false;
    }
  }

  //void DrawWidget::ScaleTimerElapsed()
  //{
  //  m_timer->stop();
  //}

  void DrawWidget::wheelEvent(QWheelEvent * e)
  {
    if (!m_isDrag && !m_isRotate)
    {
      // if we are inside the timer, cancel it
      //if (m_timer->isActive())
      //  m_timer->stop();

      //m_timer->start(m_redrawInterval);

      //m_framework->Scale(exp(e->delta() / 360.0));
      m_framework->ScaleToPoint(ScaleToPointEvent(e->pos().x(), e->pos().y(), exp(e->delta() / 360.0)));

      UpdateScaleControl();
      emit ViewportChanged();
    }
  }

  void DrawWidget::UpdateScaleControl()
  {
    if (m_pScale)
    {
      // don't send ScaleChanged
      m_pScale->SetPosWithBlockedSignals(m_framework->GetDrawScale());
    }
  }

  bool DrawWidget::Search(search::SearchParams params)
  {
    double lat, lon;
    if (m_framework->GetCurrentPosition(lat, lon))
      params.SetPosition(lat, lon);

    /// @todo This stuff doesn't work (QT4xx bug). Probably in QT5 ...
    //QLocale loc = QApplication::keyboardInputLocale();
    //params.SetInputLanguage(loc.name().toAscii().data());

    return m_framework->Search(params);
  }

  string DrawWidget::GetDistance(search::Result const & res) const
  {
    string dist;
    double lat, lon;
    if (m_framework->GetCurrentPosition(lat, lon))
    {
      double dummy;
      (void) m_framework->GetDistanceAndAzimut(res.GetFeatureCenter(), lat, lon, -1.0, dist, dummy);
    }
    return dist;
  }

  void DrawWidget::ShowSearchResult(search::Result const & res)
  {
    m_framework->ShowSearchResult(res);

    UpdateScaleControl();
  }

  void DrawWidget::CloseSearch()
  {
    m_framework->DisablePlacemark();
    setFocus();
  }

  void DrawWidget::QueryMaxScaleMode()
  {
    m_framework->XorQueryMaxScaleMode();
  }
}
