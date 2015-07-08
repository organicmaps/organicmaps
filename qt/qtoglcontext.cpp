#include "qt/qtoglcontext.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "drape/glfunctions.hpp"

QtRenderOGLContext::QtRenderOGLContext(QOpenGLContext * nativeContext, QThread * guiThread,
                                       TRegisterThreadFn const & regFn, TSwapFn const & swapFn)
  : m_surface(nativeContext->surface())
  , m_ctx(nativeContext)
  , m_guiThread(guiThread)
  , m_regFn(regFn)
  , m_swapFn(swapFn)
  , m_isRegistered(false)
  , m_shutedDown(false)
{
}

void QtRenderOGLContext::present()
{
  if (m_shutedDown)
    return;

  MoveContextOnGui();
  m_swapFn();

  makeCurrent();
}

void QtRenderOGLContext::makeCurrent()
{
  if (!m_isRegistered)
  {
    m_regFn(QThread::currentThread());
    m_isRegistered = true;
  }

  m_ctx->makeCurrent(m_surface);
}

void QtRenderOGLContext::doneCurrent()
{
  MoveContextOnGui();
}

void QtRenderOGLContext::setDefaultFramebuffer()
{
  GLFunctions::glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->defaultFramebufferObject());
}

void QtRenderOGLContext::shutDown()
{
  m_shutedDown = true;
}

void QtRenderOGLContext::MoveContextOnGui()
{
  m_ctx->doneCurrent();
  m_ctx->moveToThread(m_guiThread);
}

QtUploadOGLContext::QtUploadOGLContext(QSurface * surface, QOpenGLContext * contextToShareWith)
{
  m_surface = surface;
  m_nativeContext = new QOpenGLContext();

  ASSERT(contextToShareWith != nullptr, ());
  m_nativeContext->setShareContext(contextToShareWith);

  m_nativeContext->setFormat(contextToShareWith->format());
  VERIFY(m_nativeContext->create(), ());
}

QtUploadOGLContext::~QtUploadOGLContext()
{
  delete m_nativeContext;
}

void QtUploadOGLContext::makeCurrent()
{
  ASSERT(m_nativeContext->isValid(), ());
  m_nativeContext->makeCurrent(m_surface);
}

void QtUploadOGLContext::present()
{
  ASSERT(false, ());
}

void QtUploadOGLContext::setDefaultFramebuffer()
{
  ASSERT(false, ());
}
