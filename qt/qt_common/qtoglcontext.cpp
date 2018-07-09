#include "qt/qt_common/qtoglcontext.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/stl_add.hpp"

#include "drape/glfunctions.hpp"

namespace qt
{
namespace common
{
// QtRenderOGLContext ------------------------------------------------------------------------------
QtRenderOGLContext::QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface)
  , m_ctx(my::make_unique<QOpenGLContext>())
{
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  ASSERT(m_ctx->isValid(), ());
}

void QtRenderOGLContext::present()
{
  if (!m_resizeLock)
    lockFrame();

  m_resizeLock = false;
  GLFunctions::glFinish();

  std::swap(m_frontFrame, m_backFrame);
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

  QSize size(my::NextPowOf2(w), my::NextPowOf2(h));
  m_texRect =
      QRectF(0.0, 0.0, w / static_cast<float>(size.width()), h / static_cast<float>(size.height()));

  m_frontFrame = my::make_unique<QOpenGLFramebufferObject>(size, QOpenGLFramebufferObject::Depth);
  m_backFrame = my::make_unique<QOpenGLFramebufferObject>(size, QOpenGLFramebufferObject::Depth);
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
  if (!m_frontFrame)
    return 0;

  return m_frontFrame->texture();
}

void QtRenderOGLContext::unlockFrame()
{
  m_lock.unlock();
}

// QtUploadOGLContext ------------------------------------------------------------------------------
QtUploadOGLContext::QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface), m_ctx(my::make_unique<QOpenGLContext>())
{
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  ASSERT(m_ctx->isValid(), ());
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
}  // namespace common
}  // namespace qt
