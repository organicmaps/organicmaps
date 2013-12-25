#include "qtoglcontext.hpp"

#include "../base/assert.hpp"

QtOGLContext::QtOGLContext(QWindow * surface)
{
  m_isContextCreated = false;
  m_surface = surface;
  m_nativeContext = new QOpenGLContext(m_surface);
  m_nativeContext->setFormat(m_surface->requestedFormat());
}

QtOGLContext::QtOGLContext(QWindow * surface, QtOGLContext * contextToShareWith)
{
  m_isContextCreated = false;
  m_surface = surface;
  m_nativeContext = new QOpenGLContext(m_surface);
  m_nativeContext->setFormat(m_surface->requestedFormat());
  m_nativeContext->setShareContext(contextToShareWith->m_nativeContext);
}

void QtOGLContext::makeCurrent()
{
  if (!m_isContextCreated)
  {
    ASSERT(m_surface->isExposed(), ());
    m_nativeContext->create();
    m_isContextCreated = true;
  }
  ASSERT(m_nativeContext->isValid(), ());
  m_nativeContext->makeCurrent(m_surface);
}

void QtOGLContext::present()
{
  ASSERT(m_isContextCreated, ());
  m_nativeContext->makeCurrent(m_surface);
  m_nativeContext->swapBuffers(m_surface);
}
