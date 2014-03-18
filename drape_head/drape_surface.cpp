#include "drape_surface.hpp"

#include "../drape_frontend/viewport.hpp"

#include "../drape/utils/list_generator.hpp"
#include "../drape/shader_def.hpp"

#include "../base/stl_add.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/cmath.hpp"

#include <QtGui/QMouseEvent>


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
  m_drapeEngine.Destroy();
  m_contextFactory.Destroy();
}

void DrapeSurface::exposeEvent(QExposeEvent *e)
{
  Q_UNUSED(e);

  if (isExposed())
  {
    if (m_contextFactory.IsNull())
    {
      ThreadSafeFactory * factory = new ThreadSafeFactory(new QtOGLContextFactory(this));
      m_contextFactory = MasterPointer<OGLContextFactory>(factory);
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
    m2::PointF p = GetDevicePosition(e->pos());
    m_drapeEngine->DragStarted(p);
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
    m2::PointF p = GetDevicePosition(e->pos());
    m_drapeEngine->Drag(p);
  }
}

void DrapeSurface::mouseReleaseEvent(QMouseEvent * e)
{
  QWindow::mouseReleaseEvent(e);
  if (!isExposed())
    return;

  if (m_dragState)
  {
    m2::PointF p = GetDevicePosition(e->pos());
    m_drapeEngine->DragEnded(p);
    m_dragState = false;
  }
}

void DrapeSurface::wheelEvent(QWheelEvent * e)
{
  if (!m_dragState)
  {
    m_drapeEngine->Scale(GetDevicePosition(e->pos()), exp(e->delta() / 360.0));
  }
}

void DrapeSurface::CreateEngine()
{
  RefPointer<OGLContextFactory> f(m_contextFactory.GetRefPointer());

  float pixelRatio = devicePixelRatio();

  m_drapeEngine = MasterPointer<df::DrapeEngine>(
                    new df::DrapeEngine(f , pixelRatio, df::Viewport(pixelRatio, 0, 0, width(), height())));

  sizeChanged(0);
}

void DrapeSurface::sizeChanged(int)
{
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->Resize(width(), height());
}

m2::PointF DrapeSurface::GetDevicePosition(const QPoint & p)
{
//  qreal ratio = devicePixelRatio();
  return m2::PointF(p.x() /* ratio*/, p.y() /* ratio*/);
}
