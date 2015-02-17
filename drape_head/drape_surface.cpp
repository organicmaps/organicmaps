#include "drape_head/drape_surface.hpp"

#include "drape_frontend/viewport.hpp"

#include "base/logging.hpp"

DrapeSurface::DrapeSurface()
  : m_contextFactory(nullptr)
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
      dp::ThreadSafeFactory * factory = new dp::ThreadSafeFactory(new QtOGLContextFactory(this), false);
      m_contextFactory = dp::MasterPointer<dp::OGLContextFactory>(factory);
      CreateEngine();
    }
  }
}

void DrapeSurface::CreateEngine()
{
  dp::RefPointer<dp::OGLContextFactory> f(m_contextFactory.GetRefPointer());

  float const pixelRatio = devicePixelRatio();

  m_drapeEngine = TEnginePrt(new df::TestingEngine(f, df::Viewport(0, 0, pixelRatio * width(), pixelRatio * height()), pixelRatio));
}

void DrapeSurface::sizeChanged(int)
{
  if (!m_drapeEngine.IsNull())
  {
    float const vs = devicePixelRatio();
    int const w = width() * vs;
    int const h = height() * vs;
    m_drapeEngine->Resize(w, h);
  }
}
