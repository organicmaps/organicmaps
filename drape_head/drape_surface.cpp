#include "drape_head/drape_surface.hpp"

#include "drape_frontend/viewport.hpp"
#include "drape_frontend/map_data_provider.hpp"

#include "platform/platform.hpp"

#include "drape/shader_def.hpp"

#include "base/stl_add.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/cmath.hpp"

#include <QtGui/QMouseEvent>


DrapeSurface::DrapeSurface()
  : m_dragState(false)
  , m_contextFactory(nullptr)
{
  setSurfaceType(QSurface::OpenGLSurface);

  QObject::connect(this, SIGNAL(heightChanged(int)), this, SLOT(sizeChanged(int)));
  QObject::connect(this, SIGNAL(widthChanged(int)), this,  SLOT(sizeChanged(int)));

  ///{ Temporary initialization
  m_model.InitClassificator();
  // Platform::FilesList maps;
  // Platform & pl = GetPlatform();
  // pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, maps);

  // for_each(maps.begin(), maps.end(), bind(&model::FeaturesFetcher::RegisterMap, &m_model, _1));
  // ///}
  // ///
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
      dp::ThreadSafeFactory * factory = new dp::ThreadSafeFactory(new QtOGLContextFactory(this));
      m_contextFactory = dp::MasterPointer<dp::OGLContextFactory>(factory);
      CreateEngine();
      UpdateCoverage();
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
    m_navigator.StartDrag(p, 0);
    UpdateCoverage();
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
    m_navigator.DoDrag(p, 0);
    UpdateCoverage();
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
    m_navigator.StopDrag(p, 0, false);
    UpdateCoverage();
    m_dragState = false;
  }
}

void DrapeSurface::wheelEvent(QWheelEvent * e)
{
  if (!m_dragState)
  {
    m_navigator.ScaleToPoint(GetDevicePosition(e->pos()), exp(e->delta() / 360.0), 0);
    UpdateCoverage();
  }
}

void DrapeSurface::CreateEngine()
{
  dp::RefPointer<dp::OGLContextFactory> f(m_contextFactory.GetRefPointer());

  float pixelRatio = devicePixelRatio();

  typedef df::MapDataProvider::TReadIDsFn TReadIDsFn;
  typedef df::MapDataProvider::TReadFeaturesFn TReadFeaturesFn;
  typedef df::MapDataProvider::TReadIdCallback TReadIdCallback;
  typedef df::MapDataProvider::TReadFeatureCallback TReadFeatureCallback;

  TReadIDsFn idReadFn = [this](TReadIdCallback const & fn, m2::RectD const & r, int scale)
  {
    m_model.ForEachFeatureID(r, fn, scale);
  };

  TReadFeaturesFn featureReadFn = [this](TReadFeatureCallback const & fn, vector<FeatureID> const & ids)
  {
    m_model.ReadFeatures(fn, ids);
  };

  m_drapeEngine = TEnginePrt(new df::DrapeEngine(f, df::Viewport(0, 0, width(), height()),
                                                 df::MapDataProvider(idReadFn, featureReadFn), pixelRatio));
}

void DrapeSurface::UpdateCoverage()
{
  m_drapeEngine->UpdateCoverage(m_navigator.Screen());
}

void DrapeSurface::sizeChanged(int)
{
  if (!m_drapeEngine.IsNull())
  {
    float vs = devicePixelRatio();
    int w = width() * vs;
    int h = height() * vs;
    m_navigator.OnSize(0, 0, w, h);
    m_drapeEngine->Resize(width(), height());
    UpdateCoverage();
  }
}

m2::PointF DrapeSurface::GetDevicePosition(QPoint const & p)
{
  qreal const ratio = devicePixelRatio();
  return m2::PointF(p.x() * ratio, p.y() * ratio);
}
