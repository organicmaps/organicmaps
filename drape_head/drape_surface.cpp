#include "drape_head/drape_surface.hpp"

#include "drape_frontend/viewport.hpp"

#include "base/logging.hpp"

DrapeSurface::DrapeSurface()
{
}

DrapeSurface::~DrapeSurface()
{
  m_timer.stop();
  m_drapeEngine.reset();
}

void DrapeSurface::initializeGL()
{
  CreateEngine();
  m_timer.setInterval(1000 / 30);
  m_timer.setSingleShot(false);

  connect(&m_timer, SIGNAL(timeout()), SLOT(update()));
  m_timer.start();
}

void DrapeSurface::paintGL()
{
  m_drapeEngine->Draw();
}

void DrapeSurface::resizeGL(int width, int height)
{
  if (m_drapeEngine != nullptr)
  {
    float const vs = devicePixelRatio();
    int const w = width * vs;
    int const h = height * vs;
    m_drapeEngine->Resize(w, h);
  }
}

void DrapeSurface::CreateEngine()
{
  float const pixelRatio = devicePixelRatio();
  m_drapeEngine = make_unique_dp<df::TestingEngine>(df::Viewport(0, 0, pixelRatio * width(), pixelRatio * height()),
                                                    pixelRatio);
}
