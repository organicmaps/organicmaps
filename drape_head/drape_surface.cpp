#include "drape_surface.hpp"

#include "../drape_frontend/viewport.hpp"

#include "../drape/utils/list_generator.hpp"
#include "../drape/shader_def.hpp"

#include "../base/stl_add.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/cmath.hpp"


DrapeSurface::DrapeSurface()
  : m_contextFactory(NULL)
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
      m_drapeEngine->SetAngle(0.0);
    }
  }
}

void DrapeSurface::timerEvent(QTimerEvent * e)
{
  if (e->timerId() == m_timerID)
  {
    static const float _2pi = 2 * math::pi;
    static float angle = 0.0;
    angle += 0.0035;
    if (angle > _2pi)
      angle -= _2pi;

    // TODO this is test
    m_drapeEngine->SetAngle(angle);
  }
}

void DrapeSurface::CreateEngine()
{
  RefPointer<OGLContextFactory> f(m_contextFactory.GetRefPointer());

  float pixelRatio = devicePixelRatio();

  m_drapeEngine = MasterPointer<df::DrapeEngine>(
                    new df::DrapeEngine(f , pixelRatio, df::Viewport(pixelRatio, 0, 0, width(), height())));

  sizeChanged(0);

  //m_timerID = startTimer(1000 / 30);
}

void DrapeSurface::sizeChanged(int)
{
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->OnSizeChanged(0, 0, width(), height());
}
