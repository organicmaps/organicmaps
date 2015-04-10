#include "qt/drape_surface.hpp"

#include "drape_frontend/viewport.hpp"

#include "base/stl_add.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/cmath.hpp"

#include <QtGui/QMouseEvent>

namespace qt
{

DrapeSurface::DrapeSurface()
  : m_dragState(false)
  , m_contextFactory(NULL)
{
  setSurfaceType(QSurface::OpenGLSurface);

  QObject::connect(this, SIGNAL(heightChanged(int)), this, SLOT(sizeChanged(int)));
  QObject::connect(this, SIGNAL(widthChanged(int)), this,  SLOT(sizeChanged(int)));
}

DrapeSurface::~DrapeSurface()
{
  m_framework.PrepareToShutdown();
  m_contextFactory.Destroy();
}

void DrapeSurface::LoadState()
{
  if (!m_framework.LoadState())
    m_framework.ShowAll();
  else
    m_framework.Invalidate();
}

void DrapeSurface::SaveState()
{
  m_framework.SaveState();
}

void DrapeSurface::exposeEvent(QExposeEvent *e)
{
  Q_UNUSED(e);

  if (isExposed())
  {
    if (m_contextFactory.IsNull())
    {
      dp::ThreadSafeFactory * factory = new dp::ThreadSafeFactory(new QtOGLContextFactory(this));
      m_contextFactory = dp::MasterPointer<dp::OGLContextFactory>(factory);
      CreateEngine();
    }
  }
}

void DrapeSurface::mousePressEvent(QMouseEvent * e)
{
  QWindow::mousePressEvent(e);
  if (!isExposed())
    return;

  if (e->button() == Qt::LeftButton)
  {
    m2::PointF const p = GetDevicePosition(e->pos());
    DragEvent const event(p.x, p.y);
    m_framework.StartDrag(event);
    m_dragState = true;
  }
}

void DrapeSurface::mouseMoveEvent(QMouseEvent * e)
{
  QWindow::mouseMoveEvent(e);
  if (!isExposed())
    return;

  if (m_dragState)
  {
    m2::PointF const p = GetDevicePosition(e->pos());
    DragEvent const event(p.x, p.y);
    m_framework.DoDrag(event);
  }
}

void DrapeSurface::mouseReleaseEvent(QMouseEvent * e)
{
  QWindow::mouseReleaseEvent(e);
  if (!isExposed())
    return;

  if (m_dragState)
  {
    m2::PointF const p = GetDevicePosition(e->pos());
    DragEvent const event(p.x, p.y);
    m_framework.StopDrag(event);
    m_dragState = false;
  }
}

void DrapeSurface::wheelEvent(QWheelEvent * e)
{
  if (!m_dragState)
  {
    m2::PointF const p = GetDevicePosition(e->pos());
    ScaleToPointEvent const event(p.x, p.y, exp(e->delta() / 360.0));
    m_framework.ScaleToPoint(event, false);
  }
}

void DrapeSurface::CreateEngine()
{
  m_framework.CreateDrapeEngine(m_contextFactory.GetRefPointer(), devicePixelRatio(), width(), height());
}

void DrapeSurface::sizeChanged(int)
{
  m_framework.OnSize(width(), height());
}

m2::PointF DrapeSurface::GetDevicePosition(QPoint const & p)
{
  qreal const ratio = devicePixelRatio();
  return m2::PointF(p.x() * ratio, p.y() * ratio);
}

}
