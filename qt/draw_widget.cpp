#include "draw_widget.hpp"
#include "proxystyle.hpp"

#include "../storage/storage.hpp"

#include "../map/settings.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QSlider>

#include "../base/start_mem_debug.hpp"

using namespace storage;

namespace qt
{
  DrawWidget::DrawWidget(QWidget * pParent, Storage & storage)
    : base_type(pParent),
      m_handle(new handle_t(this)),
      m_framework(m_handle, 0),
      m_isDrag(false),
      m_redrawInterval(100),
      m_pScale(0)
  {
    m_framework.Init(storage);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(ScaleTimerElapsed()));
  }

  void DrawWidget::SetScaleControl(QSlider * pScale)
  {
    m_pScale = pScale;

    connect(m_pScale, SIGNAL(actionTriggered(int)), this, SLOT(ScaleChanged(int)));
  }

  bool DrawWidget::LoadState()
  {
    pair<uint32_t, uint32_t> widthAndHeight;
    if (!Settings::Get("DrawWidgetSize", widthAndHeight))
      return false;

    m_framework.OnSize(widthAndHeight.first, widthAndHeight.second);

    if (!m_framework.LoadState())
      return false;

    UpdateScaleControl();
    return true;
  }

  void DrawWidget::SaveState()
  {
    pair<uint32_t, uint32_t> widthAndHeight(width(), height());
    Settings::Set("DrawWidgetSize", widthAndHeight);

    m_framework.SaveState();
  }

  //void DrawWidget::ShowFeature(Feature const & p)
  //{
  //  m_framework.ShowFeature(p);
  //}

  void DrawWidget::MoveLeft()
  {
    m_framework.Move(math::pi, 0.5);
  }

  void DrawWidget::MoveRight()
  {
    m_framework.Move(0.0, 0.5);
  }

  void DrawWidget::MoveUp()
  {
    m_framework.Move(math::pi/2.0, 0.5);
  }

  void DrawWidget::MoveDown()
  {
    m_framework.Move(-math::pi/2.0, 0.5);
  }

  void DrawWidget::ScalePlus()
  {
    m_framework.Scale(2.0);
    UpdateScaleControl();
  }

  void DrawWidget::ScaleMinus()
  {
    m_framework.Scale(0.5);
    UpdateScaleControl();
  }

  void DrawWidget::ShowAll()
  {
    m_framework.ShowAll();
    UpdateScaleControl();
  }

  void DrawWidget::Repaint()
  {
    m_framework.Repaint();
  }

  void DrawWidget::ScaleChanged(int action)
  {
    if (action != QAbstractSlider::SliderNoAction)
    {
      int const oldV = m_pScale->value();
      int const newV = m_pScale->sliderPosition();
      if (oldV != newV)
      {
        double const factor = 1 << abs(oldV - newV);
        m_framework.Scale(newV > oldV ? factor : 1.0 / factor);
      }
    }
  }

  void DrawWidget::initializeGL()
  {
    widget_type::initializeGL();
    m_handle->setRenderContext(renderContext());
    m_handle->setDrawer(GetDrawer());
    m_framework.initializeGL(renderContext(), resourceManager());
  }

  void DrawWidget::DoDraw(shared_ptr<drawer_t> p)
  {
    shared_ptr<PaintEvent> paintEvent(new PaintEvent(p));
    m_framework.Paint(paintEvent);
  }

  void DrawWidget::DoResize(int w, int h)
  {
    m_framework.OnSize(w, h);
    UpdateScaleControl();
  }

  namespace
  {
    DragEvent get_drag_event(QMouseEvent * e)
    {
      QPoint const p = e->pos();
      return DragEvent(DragEvent(p.x(), p.y()));
    }
  }

  void DrawWidget::mousePressEvent(QMouseEvent * e)
  {
    base_type::mousePressEvent(e);

    if (e->button() == Qt::LeftButton)
    {
      m_framework.SetRedrawEnabled(false);
      m_framework.StartDrag(get_drag_event(e));

      setCursor(Qt::CrossCursor);
      m_isDrag = true;
    }
  }

  void DrawWidget::mouseDoubleClickEvent(QMouseEvent * e)
  {
    base_type::mouseDoubleClickEvent(e);

    if (e->button() == Qt::LeftButton)
    {
      StopDragging(e);

      m_framework.ScaleToPoint(ScaleToPointEvent(e->pos().x(), e->pos().y(), 1.5));

      UpdateScaleControl();
    }
  }

  void DrawWidget::mouseMoveEvent(QMouseEvent * e)
  {
    base_type::mouseMoveEvent(e);

    if (m_isDrag)
      m_framework.DoDrag(get_drag_event(e));
  }

  void DrawWidget::mouseReleaseEvent(QMouseEvent * e)
  {
    base_type::mouseReleaseEvent(e);

    StopDragging(e);
  }

  void DrawWidget::StopDragging(QMouseEvent * e)
  {
    if (m_isDrag && e->button() == Qt::LeftButton)
    {
      m_framework.SetRedrawEnabled(true);
      m_framework.StopDrag(get_drag_event(e));

      setCursor(Qt::ArrowCursor);
      m_isDrag = false;
    }
  }

  void DrawWidget::ScaleTimerElapsed()
  {
    m_framework.SetRedrawEnabled(true);
    m_timer->stop();
  }

  void DrawWidget::wheelEvent(QWheelEvent * e)
  {
    if (!m_isDrag)
    {
      /// if we are inside the timer, cancel it
      if (m_timer->isActive())
        m_timer->stop();

      m_framework.SetRedrawEnabled(false);
      m_timer->start(m_redrawInterval);
      //m_framework.Scale(exp(e->delta() / 360.0));
      m_framework.ScaleToPoint(ScaleToPointEvent(e->pos().x(), e->pos().y(), exp(e->delta() / 360.0)));
      UpdateScaleControl();
    }
  }

  void DrawWidget::UpdateScaleControl()
  {
    if (m_pScale)
    {
      // don't send ScaleChanged

      bool const b = m_pScale->signalsBlocked();
      m_pScale->blockSignals(true);

      m_pScale->setSliderPosition(m_framework.GetCurrentScale());

      m_pScale->blockSignals(b);
    }
  }
}
