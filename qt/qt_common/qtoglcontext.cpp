#include "qt/qt_common/qtoglcontext.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

#include "drape/glfunctions.hpp"

#include <memory>

namespace qt
{
namespace common
{
// QtRenderOGLContext ------------------------------------------------------------------------------
QtRenderOGLContext::QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface)
  , m_ctx(std::make_unique<QOpenGLContext>())
{
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  ASSERT(m_ctx->isValid(), ());
}

void QtRenderOGLContext::Present()
{
  if (!m_resizeLock)
    LockFrame();

  m_resizeLock = false;
  GLFunctions::glFinish();

  std::swap(m_frontFrame, m_backFrame);
  UnlockFrame();
}

void QtRenderOGLContext::MakeCurrent()
{
  VERIFY(m_ctx->makeCurrent(m_surface), ());
}

void QtRenderOGLContext::DoneCurrent()
{
  m_ctx->doneCurrent();
}

void QtRenderOGLContext::SetDefaultFramebuffer()
{
  if (m_backFrame == nullptr)
    return;

  m_backFrame->bind();
}

void QtRenderOGLContext::Resize(int w, int h)
{
  LockFrame();
  m_resizeLock = true;

  QSize size(my::NextPowOf2(w), my::NextPowOf2(h));
  m_texRect =
      QRectF(0.0, 0.0, w / static_cast<float>(size.width()), h / static_cast<float>(size.height()));

  m_frontFrame = std::make_unique<QOpenGLFramebufferObject>(size, QOpenGLFramebufferObject::Depth);
  m_backFrame = std::make_unique<QOpenGLFramebufferObject>(size, QOpenGLFramebufferObject::Depth);
}

void QtRenderOGLContext::LockFrame()
{
  m_lock.lock();
}

QRectF const & QtRenderOGLContext::GetTexRect() const
{
  return m_texRect;
}

GLuint QtRenderOGLContext::GetTextureHandle() const
{
  if (!m_frontFrame)
    return 0;

  return m_frontFrame->texture();
}

void QtRenderOGLContext::UnlockFrame()
{
  m_lock.unlock();
}

// QtUploadOGLContext ------------------------------------------------------------------------------
QtUploadOGLContext::QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface), m_ctx(std::make_unique<QOpenGLContext>())
{
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  ASSERT(m_ctx->isValid(), ());
}

void QtUploadOGLContext::MakeCurrent()
{
  m_ctx->makeCurrent(m_surface);
}

void QtUploadOGLContext::DoneCurrent()
{
  m_ctx->doneCurrent();
}

void QtUploadOGLContext::Present()
{
  ASSERT(false, ());
}

void QtUploadOGLContext::SetDefaultFramebuffer()
{
  ASSERT(false, ());
}
}  // namespace common
}  // namespace qt
