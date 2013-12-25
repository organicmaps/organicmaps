#include "drape_surface.hpp"

#include "../base/logging.hpp"

DrapeSurface::DrapeSurface()
  : m_contextFactory(NULL)
{
  setSurfaceType(QSurface::OpenGLSurface);
}

DrapeSurface::~DrapeSurface()
{
  delete m_contextFactory;
}

void DrapeSurface::exposeEvent(QExposeEvent *e)
{
  Q_UNUSED(e);

  if (isExposed() && m_contextFactory == NULL)
  {
    m_contextFactory = new QtOGLContextFactory(this);
    CreateEngine();
  }
}
