#include "qtoglcontext.hpp"

#include "../base/assert.hpp"

QtOGLContext::QtOGLContext(QWindow * surface, QtOGLContext * contextToShareWith)
{
  m_isContextCreated = false;
  m_surface = surface;
  m_nativeContext = new QOpenGLContext(m_surface);
  m_nativeContext->setFormat(m_surface->requestedFormat());

  if  (contextToShareWith != NULL)
    m_nativeContext->setShareContext(contextToShareWith->m_nativeContext);
}

QtOGLContext::~QtOGLContext()
{
  delete m_nativeContext;
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


void QtOGLContext::setDefaultFramebuffer()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
