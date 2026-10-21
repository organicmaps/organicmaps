#include "qt/qt_common/qtoglcontext.hpp"

#include "base/assert.hpp"

#include "drape/gl_functions.hpp"

#include <memory>

namespace qt
{
namespace common
{
QtRenderOGLContext::QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface)
  , m_ctx(std::make_unique<QOpenGLContext>())
  , m_isContextAvailable(false)
{
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  CHECK(m_ctx->isValid(), ());
}

void QtRenderOGLContext::Present()
{
  GLFunctions::glFinish();

  std::lock_guard<std::mutex> lock(m_frameMutex);
  if (!m_backFrame)
    return;
  std::swap(m_frontFrame, m_backFrame);
  m_frameUpdated = true;
  if (m_onPresent)
    m_onPresent();
}

void QtRenderOGLContext::SetPresentListener(std::function<void()> fn)
{
  std::lock_guard<std::mutex> lock(m_frameMutex);
  m_onPresent = std::move(fn);
}

void QtRenderOGLContext::MakeCurrent()
{
  CHECK(m_ctx->makeCurrent(m_surface), ());
  m_isContextAvailable = true;
}

void QtRenderOGLContext::DoneCurrent()
{
  m_isContextAvailable = false;
  m_ctx->doneCurrent();
}

void QtRenderOGLContext::SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer)
{
  if (framebuffer)
    framebuffer->Bind();
  else
  {
    PrepareBackFrame();
    if (m_backFrame)
      m_backFrame->bind();
  }
}

void QtRenderOGLContext::Resize(uint32_t w, uint32_t h)
{
  CHECK(m_isContextAvailable, ());
  ASSERT(QOpenGLContext::currentContext() == m_ctx.get(), ());

  m_size = QSize(static_cast<int>(w), static_cast<int>(h));
}

void QtRenderOGLContext::PrepareBackFrame()
{
  ASSERT(QOpenGLContext::currentContext() == m_ctx.get(), ());
  if (m_size.isEmpty())
    return;
  if (!m_backFrame || m_backFrame->size() != m_size)
  {
    m_backFrame = std::make_unique<QOpenGLFramebufferObject>(m_size, QOpenGLFramebufferObject::Depth);
    CHECK(m_backFrame->isValid(), (m_size.width(), m_size.height()));
  }
}

bool QtRenderOGLContext::AcquireFrame()
{
  if (!m_isContextAvailable)
    return false;

  ASSERT(QOpenGLContext::currentContext(), ());
  ASSERT(QOpenGLContext::currentContext() != m_ctx.get(), ());
  ASSERT(QOpenGLContext::areSharing(QOpenGLContext::currentContext(), m_ctx.get()), ());

  std::unique_lock lock(m_frameMutex);
  if (!m_frameUpdated)
    return m_acquiredFrame != nullptr;

  // Finish the consumer's reads before returning its texture to the producer.
  if (m_acquiredFrame)
  {
    // The consumer keeps ownership while the producer can publish a newer front frame.
    lock.unlock();
    GLFunctions::glFinish();
    lock.lock();
  }
  std::swap(m_acquiredFrame, m_frontFrame);
  m_frameUpdated = false;

  return true;
}

QRectF const & QtRenderOGLContext::GetTexRect() const
{
  static QRectF const kUnitRect(0.0, 0.0, 1.0, 1.0);
  return kUnitRect;
}

GLuint QtRenderOGLContext::GetTextureHandle() const
{
  if (!m_acquiredFrame)
    return 0;

  return m_acquiredFrame->texture();
}

QtUploadOGLContext::QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface)
  : m_surface(surface)
  , m_ctx(std::make_unique<QOpenGLContext>())
{
  m_ctx->setFormat(rootContext->format());
  m_ctx->setShareContext(rootContext);
  m_ctx->create();
  CHECK(m_ctx->isValid(), ());
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
  CHECK(false, ());
}

void QtUploadOGLContext::SetFramebuffer(ref_ptr<dp::BaseFramebuffer>)
{
  CHECK(false, ());
}
}  // namespace common
}  // namespace qt
