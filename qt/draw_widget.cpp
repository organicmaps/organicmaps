#include "draw_widget.hpp"
#include "slider_ctrl.hpp"

#include "../map/render_policy.hpp"

#include "../search/result.hpp"

#include "../gui/controller.hpp"

#include "../graphics/opengl/opengl.hpp"
#include "../graphics/depth_constants.hpp"

#include "../platform/settings.hpp"
#include "../platform/platform.hpp"

#include <QtCore/QLocale>

#include <QtGui/QMouseEvent>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QMenu>
  #include <QtGui/QApplication>
  #include <QtGui/QDesktopWidget>
#else
  #include <QtWidgets/QMenu>
  #include <QtWidgets/QApplication>
  #include <QtWidgets/QDesktopWidget>
#endif

namespace qt
{
  const unsigned LONG_TOUCH_MS = 1000;
  const unsigned SHORT_TOUCH_MS = 250;

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

  void DummyUserMark(UserMarkCopy * ){}
  void DummyDismiss() {}

  DrawWidget::DrawWidget(QWidget * pParent)
    : QGLWidget(pParent),
      m_isInitialized(false),
      m_isTimerStarted(false),
      m_framework(new Framework()),
      m_isDrag(false),
      m_isRotate(false),
      //m_redrawInterval(100),
      m_ratio(1.0),
      m_pScale(0)
  {
    // Initialize with some stubs for test.
    PinClickManager & manager = GetBalloonManager();
    manager.ConnectUserMarkListener(&DummyUserMark);
    manager.ConnectDismissListener(&DummyDismiss);
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

  void DrawWidget::LoadState()
  {
    m_framework->LoadBookmarks();

    if (!m_framework->LoadState())
      ShowAll();
    else
    {
      UpdateNow();
      UpdateScaleControl();
    }
  }

  void DrawWidget::SaveState()
  {
    m_framework->SaveState();
  }

  void DrawWidget::MoveLeft()
  {
    m_framework->Move(math::pi, 0.5);
  }

  void DrawWidget::MoveRight()
  {
    m_framework->Move(0.0, 0.5);
  }

  void DrawWidget::MoveUp()
  {
    m_framework->Move(math::pi/2.0, 0.5);
  }

  void DrawWidget::MoveDown()
  {
    m_framework->Move(-math::pi/2.0, 0.5);
  }

  void DrawWidget::ScalePlus()
  {
    m_framework->Scale(2.0);
    UpdateScaleControl();
  }

  void DrawWidget::ScaleMinus()
  {
    m_framework->Scale(0.5);
    UpdateScaleControl();
  }

  void DrawWidget::ScalePlusLight()
  {
    m_framework->Scale(1.5);
    UpdateScaleControl();
  }

  void DrawWidget::ScaleMinusLight()
  {
    m_framework->Scale(2.0/3.0);
    UpdateScaleControl();
  }

  void DrawWidget::ShowAll()
  {
    m_framework->ShowAll();
    UpdateScaleControl();
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
        m_framework->Scale(factor);
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
      rmParams.m_texFormat = graphics::Data8Bpp;
      rmParams.m_texRtFormat = graphics::Data4Bpp;
      rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

      RenderPolicy::Params rpParams;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
      m_ratio = dynamic_cast<QApplication*>(qApp)->devicePixelRatio();
#endif

      QRect const & geometry = QApplication::desktop()->geometry();
      rpParams.m_screenWidth = L2D(geometry.width());
      rpParams.m_screenHeight = L2D(geometry.height());

      if (m_ratio >= 1.5 || QApplication::desktop()->physicalDpiX() >= 180)
        rpParams.m_density = graphics::EDensityXHDPI;
      else
        rpParams.m_density = graphics::EDensityMDPI;

      rpParams.m_videoTimer = m_videoTimer.get();
      rpParams.m_useDefaultFB = true;
      rpParams.m_rmParams = rmParams;
      rpParams.m_primaryRC = primaryRC;
      rpParams.m_skinName = "basic.skn";

      try
      {
        m_framework->SetRenderPolicy(CreateRenderPolicy(rpParams));
        m_framework->InitGuiSubsystem();
      }
      catch (graphics::gl::platform_unsupported const & e)
      {
        LOG(LERROR, ("OpenGL platform is unsupported, reason: ", e.what()));
        /// @todo Show "Please Update Drivers" dialog and close the program.
      }

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

  void DrawWidget::StartPressTask(m2::PointD const & pt, unsigned ms)
  {
    if (KillPressTask())
      m_scheduledTask.reset(new ScheduledTask(bind(&DrawWidget::OnPressTaskEvent, this, pt, ms), ms));
  }

  bool DrawWidget::KillPressTask()
  {
    if (m_scheduledTask)
    {
      if (!m_scheduledTask->CancelNoBlocking())
      {
        // The task is already running - skip new task.
        return false;
      }

      m_scheduledTask.reset();
    }
    return true;
  }

  void DrawWidget::OnPressTaskEvent(m2::PointD const & pt, unsigned ms)
  {
    m_wasLongClick = (ms == LONG_TOUCH_MS);
    GetBalloonManager().OnShowMark(m_framework->GetUserMark(pt, m_wasLongClick));
  }

  m2::PointD DrawWidget::GetDevicePoint(QMouseEvent * e) const
  {
    return m2::PointD(L2D(e->x()), L2D(e->y()));
  }

  DragEvent DrawWidget::GetDragEvent(QMouseEvent * e) const
  {
    return DragEvent(L2D(e->x()), L2D(e->y()));
  }

  RotateEvent DrawWidget::GetRotateEvent(QPoint const & pt) const
  {
    QPoint const center = rect().center();
    return RotateEvent(L2D(center.x()), L2D(center.y()),
                       L2D(pt.x()), L2D(pt.y()));
  }

  namespace
  {
    void add_string(QMenu & menu, string const & s)
    {
      (void)menu.addAction(QString::fromUtf8(s.c_str()));
    }
  }

  void DrawWidget::mousePressEvent(QMouseEvent * e)
  {
    QGLWidget::mousePressEvent(e);

    KillPressTask();

    m2::PointD const pt = GetDevicePoint(e);

    if (e->button() == Qt::LeftButton)
    {
      if (m_framework->GetGuiController()->OnTapStarted(pt))
        return;

      if (e->modifiers() & Qt::ControlModifier)
      {
        // starting rotation
        m_framework->StartRotate(GetRotateEvent(e->pos()));

        setCursor(Qt::CrossCursor);
        m_isRotate = true;
      }
      else
      {
        // init press task params
        m_taskPoint = pt;
        m_wasLongClick = false;
        m_isCleanSingleClick = true;

        StartPressTask(pt, LONG_TOUCH_MS);

        // starting drag
        m_framework->StartDrag(GetDragEvent(e));

        setCursor(Qt::CrossCursor);
        m_isDrag = true;
      }
    }
    else if (e->button() == Qt::RightButton)
    {
      // show feature types
      QMenu menu;

      // Get POI under cursor or nearest address by point.
      m2::PointD dummy;
      search::AddressInfo info;
      if (m_framework->GetVisiblePOI(pt, dummy, info))
      {
        add_string(menu, "POI");
      }
      else
      {
        m_framework->GetAddressInfoForPixelPoint(pt, info);
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

      menu.exec(e->pos());
    }
  }

  void DrawWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    QGLWidget::mouseDoubleClickEvent(e);

    KillPressTask();
    m_isCleanSingleClick = false;

    if (e->button() == Qt::LeftButton)
    {
      StopDragging(e);

      m_framework->ScaleToPoint(ScaleToPointEvent(L2D(e->x()), L2D(e->y()), 1.5));

      UpdateScaleControl();
    }
  }

  void DrawWidget::mouseMoveEvent(QMouseEvent * e)
  {
    QGLWidget::mouseMoveEvent(e);

    m2::PointD const pt = GetDevicePoint(e);
    if (!pt.EqualDxDy(m_taskPoint, m_framework->GetVisualScale() * 10.0))
    {
      // moved far from start point - do not show balloon
      m_isCleanSingleClick = false;
      KillPressTask();
    }

    if (m_framework->GetGuiController()->OnTapMoved(pt))
      return;

    if (m_isDrag)
      m_framework->DoDrag(GetDragEvent(e));

    if (m_isRotate)
      m_framework->DoRotate(GetRotateEvent(e->pos()));
  }

  void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    QGLWidget::mouseReleaseEvent(e);

    m2::PointD const pt = GetDevicePoint(e);
    if (m_framework->GetGuiController()->OnTapEnded(pt))
      return;

    if (!m_wasLongClick && m_isCleanSingleClick)
      StartPressTask(pt, SHORT_TOUCH_MS);
    else
      m_wasLongClick = false;

    StopDragging(e);
    StopRotating(e);
  }

  void DrawWidget::keyReleaseEvent(QKeyEvent * e)
  {
    QGLWidget::keyReleaseEvent(e);

    StopRotating(e);
  }

  void DrawWidget::StopRotating(QMouseEvent * e)
  {
    if (m_isRotate && (e->button() == Qt::LeftButton))
    {
      m_framework->StopRotate(GetRotateEvent(e->pos()));

      setCursor(Qt::ArrowCursor);
      m_isRotate = false;
    }
  }

  void DrawWidget::StopRotating(QKeyEvent * e)
  {
    if (m_isRotate && (e->key() == Qt::Key_Control))
    {
      m_framework->StopRotate(GetRotateEvent(mapFromGlobal(QCursor::pos())));
    }
  }

  void DrawWidget::StopDragging(QMouseEvent * e)
  {
    if (m_isDrag && e->button() == Qt::LeftButton)
    {
      m_framework->StopDrag(GetDragEvent(e));

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
      m_framework->ScaleToPoint(ScaleToPointEvent(L2D(e->x()), L2D(e->y()), exp(e->delta() / 360.0)));

      UpdateScaleControl();
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

    // This stuff always returns system language (not keyboard input language).
    /*
    QInputMethod const * pIM = QApplication::inputMethod();
    if (pIM)
    {
      string const lang = pIM->locale().name().toStdString();
      LOG(LDEBUG, ("QT input language", lang));
      params.SetInputLanguage(lang);
    }
    */

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
    setFocus();
  }

  void DrawWidget::QueryMaxScaleMode()
  {
    m_framework->XorQueryMaxScaleMode();
  }
}
