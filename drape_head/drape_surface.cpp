#include "drape_surface.hpp"

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
  m_contextFactory.Destroy();
}

void DrapeSurface::exposeEvent(QExposeEvent *e)
{
  Q_UNUSED(e);

  if (isExposed())
  {
    if (m_contextFactory.IsNull())
    {
      m_contextFactory = MasterPointer<QtOGLContextFactory>(new QtOGLContextFactory(this));
      CreateEngine();
    }
  }
}

void DrapeSurface::timerEvent(QTimerEvent * e)
{
  if (e->timerId() == m_timerID)
  {
    static const float _2pi = 2 * math::pi;
    static float angle = 0.0;
    angle += 0.035;
    if (angle > _2pi)
      angle -= _2pi;

    // TODO this is test
    m_drapeEngine->SetAngle(angle);
  }
}

void DrapeSurface::CreateEngine()
{
  RefPointer<OGLContextFactory> f(m_contextFactory.GetRefPointer());

  m_drapeEngine = MasterPointer<df::DrapeEngine>(
                    new df::DrapeEngine(f , devicePixelRatio(), width(), height()));

  m_timerID = startTimer(1000 / 30);
}

void DrapeSurface::sizeChanged(int)
{
  if (!m_drapeEngine.IsNull())
    m_drapeEngine->OnSizeChanged(0, 0, width(), height());
}
