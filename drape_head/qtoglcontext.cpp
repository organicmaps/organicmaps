#include "drape_head/qtoglcontext.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "drape/glfunctions.hpp"

QtOGLContext::QtOGLContext(QWindow * surface, QtOGLContext * contextToShareWith)
{
  m_isContextCreated = false;
  m_surface = surface;
  m_nativeContext = new QOpenGLContext();

  if  (contextToShareWith != NULL)
    m_nativeContext->setShareContext(contextToShareWith->m_nativeContext);

  m_nativeContext->setFormat(m_surface->requestedFormat());
  ASSERT(m_surface->isExposed(), ());
  VERIFY(m_nativeContext->create(), ());
}

QtOGLContext::~QtOGLContext()
{
  delete m_nativeContext;
}

void QtOGLContext::makeCurrent()
{
  ASSERT(m_nativeContext->isValid(), ());
  m_nativeContext->makeCurrent(m_surface);

#ifdef DEBUG
  LOG(LDEBUG, ("Current context : ", m_nativeContext));
  QList<QOpenGLContext *> list = QOpenGLContextGroup::currentContextGroup()->shares();
  for (int i = 0; i < list.size(); ++i)
    LOG(LDEBUG, ("Share context : ", list[i]));
#endif
}

void QtOGLContext::present()
{
  m_nativeContext->makeCurrent(m_surface);
  m_nativeContext->swapBuffers(m_surface);
}

void QtOGLContext::setDefaultFramebuffer()
{
  GLFunctions::glBindFramebuffer(GL_FRAMEBUFFER, m_nativeContext->defaultFramebufferObject());
}
