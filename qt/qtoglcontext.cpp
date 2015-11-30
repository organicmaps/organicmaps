#include "qt/qtoglcontext.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

#include "drape/glfunctions.hpp"

QtRenderOGLContext::QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface)
{
  m_ctx = new QOpenGLContext();
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  ASSERT(m_ctx->isValid(), ());
}

QtRenderOGLContext::~QtRenderOGLContext()
{
  delete m_frontFrame;
  delete m_backFrame;
  delete m_ctx;
}

void QtRenderOGLContext::present()
{
  if (!m_resizeLock)
    lockFrame();

  m_resizeLock = false;
  GLFunctions::glFinish();

  swap(m_frontFrame, m_backFrame);
  unlockFrame();
}

void QtRenderOGLContext::makeCurrent()
{
  VERIFY(m_ctx->makeCurrent(m_surface), ());
}

void QtRenderOGLContext::doneCurrent()
{
  m_ctx->doneCurrent();
}

void QtRenderOGLContext::setDefaultFramebuffer()
{
  if (m_backFrame == nullptr)
    return;

  m_backFrame->bind();
}

void QtRenderOGLContext::resize(int w, int h)
{
  lockFrame();
  m_resizeLock = true;

  delete m_frontFrame;
  delete m_backFrame;

  QSize size(my::NextPowOf2(w), my::NextPowOf2(h));
  m_texRect = QRectF(0.0, 0.0, w / (float)size.width(), h / (float)size.height());

  m_frontFrame = new QOpenGLFramebufferObject(size, QOpenGLFramebufferObject::Depth);
  m_backFrame = new QOpenGLFramebufferObject(size, QOpenGLFramebufferObject::Depth);
}

void QtRenderOGLContext::lockFrame()
{
  m_lock.lock();
}

QRectF const & QtRenderOGLContext::getTexRect() const
{
  return m_texRect;
}

GLuint QtRenderOGLContext::getTextureHandle() const
{
  if (m_frontFrame == nullptr)
    return 0;

  return m_frontFrame->texture();
}

void QtRenderOGLContext::unlockFrame()
{
  m_lock.unlock();
}

QtUploadOGLContext::QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface)
{
  m_ctx = new QOpenGLContext();
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  ASSERT(m_ctx->isValid(), ());
}

QtUploadOGLContext::~QtUploadOGLContext()
{
  delete m_ctx;
}

void QtUploadOGLContext::makeCurrent()
{
  m_ctx->makeCurrent(m_surface);
}

void QtUploadOGLContext::doneCurrent()
{
  m_ctx->doneCurrent();
}

void QtUploadOGLContext::present()
{
  ASSERT(false, ());
}

void QtUploadOGLContext::setDefaultFramebuffer()
{
  ASSERT(false, ());
}
