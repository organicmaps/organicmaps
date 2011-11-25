#include "draw_widget.hpp"
#include "proxystyle.hpp"
#include "slider_ctrl.hpp"

#include "../map/framework_factory.hpp"
#include "../map/render_policy.hpp"

#include "../yg/internal/opengl.hpp"

#include "../platform/settings.hpp"

#include <QtGui/QMouseEvent>


using namespace storage;

namespace qt
{
  QtVideoTimer::QtVideoTimer(DrawWidget * w, TFrameFn frameFn)
    : ::VideoTimer(frameFn), m_widget(w)
  {}

  void QtVideoTimer::start()
  {
    m_timer = new QTimer();
    m_widget->connect(m_timer, SIGNAL(timeout()), m_widget, SLOT(AnimTimerElapsed()));
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

  DrawWidget::DrawWidget(QWidget * pParent)
    : QGLWidget(pParent),
      m_isInitialized(false),
      m_isTimerStarted(false),
      m_framework(FrameworkFactory::CreateFramework()),
      m_isDrag(false),
      m_isRotate(false),
      m_redrawInterval(100),
      m_pScale(0)
  {
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(ScaleTimerElapsed()));
  }

  void DrawWidget::PrepareShutdown()
  {
    m_framework->PrepareToShutdown();
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
    //m_framework.UpdateNow();

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
#ifdef OMIM_OS_MAC
    return CreateAppleVideoTimer(bind(&DrawWidget::DrawFrame, this));
#else
    return new QtVideoTimer(this, bind(&DrawWidget::DrawFrame, this));
#endif
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
#ifdef OMIM_OS_WINDOWS
      win32::InitOpenGL();
#endif

    /// we'll perform swap by ourselves, see issue #333
    setAutoBufferSwap(false);

    if (!m_isInitialized)
    {
      m_videoTimer.reset(CreateVideoTimer());

      shared_ptr<qt::gl::RenderContext> primaryRC(new qt::gl::RenderContext(this));

      yg::ResourceManager::Params rmParams;
      rmParams.m_rtFormat = yg::Rt8Bpp;
      rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

      m_framework->SetRenderPolicy(CreateRenderPolicy(m_videoTimer.get(), true, rmParams, primaryRC));

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
    if ((m_isInitialized) && (!m_isTimerStarted))
    {
      /// timer should be started upon the first repaint
      /// request to fully initialized GLWidget.
      m_isTimerStarted = true;
      m_framework->SetUpdatesEnabled(true);
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

      /// swapping buffers before ending the frame, see issue #333
      swapBuffers();

      m_framework->EndPaint(paintEvent);
      doneCurrent();
    }
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
  }

  void DrawWidget::mousePressEvent(QMouseEvent * e)
  {
    QGLWidget::mousePressEvent(e);

    if (e->button() == Qt::LeftButton)
    {
      if (e->modifiers() & Qt::ControlModifier)
      {
        /// starting rotation
        m_framework->StartRotate(get_rotate_event(e->pos(), this->rect().center()));
        setCursor(Qt::CrossCursor);
        m_isRotate = true;
      }
      else
      {
        /// starting drag
        m_framework->StartDrag(get_drag_event(e));

        setCursor(Qt::CrossCursor);
        m_isDrag = true;
      }
    }
  }

  void DrawWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    QGLWidget::mouseDoubleClickEvent(e);

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

    if (m_isDrag)
      m_framework->DoDrag(get_drag_event(e));

    if (m_isRotate)
      m_framework->DoRotate(get_rotate_event(e->pos(), this->rect().center()));
  }

  void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    QGLWidget::mouseReleaseEvent(e);

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

  void DrawWidget::ScaleTimerElapsed()
  {
    m_timer->stop();
  }

  void DrawWidget::AnimTimerElapsed()
  {
    DrawFrame();
  }

  void DrawWidget::wheelEvent(QWheelEvent * e)
  {
    if ((!m_isDrag) && (!m_isRotate))
    {
      /// if we are inside the timer, cancel it
      if (m_timer->isActive())
        m_timer->stop();

      m_timer->start(m_redrawInterval);
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
      m_pScale->SetPosWithBlockedSignals(m_framework->GetCurrentScale());
    }
  }

  void DrawWidget::Search(const string & text, SearchCallbackT callback)
  {
    m_framework->Search(text, callback);
  }

  void DrawWidget::ShowFeature(m2::RectD const & rect)
  {
    m_framework->ShowRect(rect);
    UpdateScaleControl();
  }
}
