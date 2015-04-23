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
}

void DrapeSurface::exposeEvent(QExposeEvent *e)
{
  Q_UNUSED(e);

  if (isExposed())
  {
    if (m_contextFactory == nullptr)
    {
      m_contextFactory = make_unique_dp<dp::ThreadSafeFactory>(new QtOGLContextFactory(this), false);
      CreateEngine();
    }
  }
}

void DrapeSurface::CreateEngine()
{
  float const pixelRatio = devicePixelRatio();
  m_drapeEngine = make_unique_dp<df::TestingEngine>(make_ref(m_contextFactory),
                                                    df::Viewport(0, 0, pixelRatio * width(), pixelRatio * height()),
                                                    pixelRatio);
}

void DrapeSurface::sizeChanged(int)
{
  if (m_drapeEngine != nullptr)
  {
    float const vs = devicePixelRatio();
    int const w = width() * vs;
    int const h = height() * vs;
    m_drapeEngine->Resize(w, h);
  }
}
