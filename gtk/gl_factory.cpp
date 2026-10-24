#include "gl_factory.hpp"

#include "drape/framebuffer.hpp"
#include "drape/gl_functions.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <thread>

namespace gtk
{
namespace
{
uint64_t constexpr kFenceTimeoutNs = 1'000'000'000;
}  // namespace

OffscreenFrame::OffscreenFrame(uint32_t w, uint32_t h) : m_width(w), m_height(h)
{
  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(w), static_cast<GLsizei>(h), 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenRenderbuffers(1, &m_depthStencil);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencil);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthStencil);
  GLenum const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  CHECK_EQUAL(status, GL_FRAMEBUFFER_COMPLETE, ("Offscreen framebuffer is incomplete", w, h));
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

OffscreenFrame::~OffscreenFrame()
{
  if (m_consumerFence)
  {
    // Frames are deleted rarely (resize, shutdown): a CPU wait is fine.
    glClientWaitSync(m_consumerFence, 0, kFenceTimeoutNs);
    glDeleteSync(m_consumerFence);
  }
  glDeleteFramebuffers(1, &m_fbo);
  glDeleteRenderbuffers(1, &m_depthStencil);
  glDeleteTextures(1, &m_texture);
}

void OffscreenFrame::SetConsumerFence(GLsync fence)
{
  if (m_consumerFence)
    glDeleteSync(m_consumerFence);
  m_consumerFence = fence;
}

void OffscreenFrame::WaitForConsumer()
{
  if (!m_consumerFence)
    return;
  glWaitSync(m_consumerFence, 0, GL_TIMEOUT_IGNORED);
  glDeleteSync(m_consumerFence);
  m_consumerFence = nullptr;
}

GtkRenderContext::GtkRenderContext(GdkGLContext * ctx, uint32_t w, uint32_t h, std::function<void()> onFrameReady)
  : m_ctx(GDK_GL_CONTEXT(g_object_ref(ctx)))
  , m_initialWidth(w)
  , m_initialHeight(h)
  , m_onFrameReady(std::move(onFrameReady))
{}

GtkRenderContext::~GtkRenderContext()
{
  // The render thread is joined by now. FBOs are not shared: delete the frames in their context.
  gdk_gl_context_make_current(m_ctx);
  m_back.reset();
  m_front.reset();
  m_acquired.reset();
  gdk_gl_context_clear_current();
  g_object_unref(m_ctx);
}

void GtkRenderContext::MakeCurrent()
{
  gdk_gl_context_make_current(m_ctx);
  // The engine draws an empty frame before the first Resize(); the context itself is surfaceless.
  if (!m_back)
    Resize(m_initialWidth, m_initialHeight);
}

void GtkRenderContext::DoneCurrent()
{
  gdk_gl_context_clear_current();
}

void GtkRenderContext::Present()
{
  // Frames are consumed by another context, so the work must be finished before handing it over.
  GLFunctions::glFinish();

  // A surfaceless context has no swap interval. Pace the producer without waiting for GTK,
  // whose main thread may be waiting for the renderer during suspend, resize or shutdown.
  std::this_thread::sleep_until(m_nextPresent);
  m_nextPresent =
      std::chrono::steady_clock::now() + std::chrono::microseconds(m_frameIntervalUs.load(std::memory_order_relaxed));

  {
    std::lock_guard lock(m_frameMutex);
    std::swap(m_front, m_back);
    m_frameUpdated = true;
    // The new back frame may be the one the GTK context has just drawn.
    m_back->WaitForConsumer();
  }
  if (m_onFrameReady)
    m_onFrameReady();
}

void GtkRenderContext::SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer)
{
  if (framebuffer)
    framebuffer->Bind();
  else if (m_back)
    glBindFramebuffer(GL_FRAMEBUFFER, m_back->m_fbo);
}

void GtkRenderContext::Resize(uint32_t w, uint32_t h)
{
  std::lock_guard lock(m_frameMutex);
  if (m_back && m_back->m_width == w && m_back->m_height == h)
    return;

  // The main thread is not drawing (it holds the mutex while it does), and the old frames wait
  // for their consumer fences before deletion.
  m_back = std::make_unique<OffscreenFrame>(w, h);
  m_front = std::make_unique<OffscreenFrame>(w, h);
  m_acquired = std::make_unique<OffscreenFrame>(w, h);
  m_frameUpdated = false;
  m_hasFrame = false;
  glBindFramebuffer(GL_FRAMEBUFFER, m_back->m_fbo);
}

bool GtkRenderContext::DrawFrame(int64_t frameIntervalUs, std::function<void(GLuint texture)> const & draw)
{
  ASSERT_GREATER(frameIntervalUs, 0, ());
  m_frameIntervalUs.store(frameIntervalUs, std::memory_order_relaxed);
  std::lock_guard lock(m_frameMutex);
  if (m_frameUpdated)
  {
    std::swap(m_acquired, m_front);
    m_frameUpdated = false;
    m_hasFrame = true;
  }
  if (!m_hasFrame)
    return false;
  draw(m_acquired->m_texture);
  m_acquired->SetConsumerFence(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
  // Another context waits for the fence: it must reach the GPU.
  glFlush();
  return true;
}

GtkUploadContext::GtkUploadContext(GdkGLContext * ctx) : m_ctx(GDK_GL_CONTEXT(g_object_ref(ctx))) {}

GtkUploadContext::~GtkUploadContext()
{
  g_object_unref(m_ctx);
}

void GtkUploadContext::MakeCurrent()
{
  gdk_gl_context_make_current(m_ctx);
}

void GtkUploadContext::DoneCurrent()
{
  gdk_gl_context_clear_current();
}

GtkContextFactory::GtkContextFactory(GdkDisplay * display, uint32_t w, uint32_t h, std::function<void()> onFrameReady)
{
  m_drawCtx = CreateContext(display);
  m_uploadCtx = CreateContext(display);
  if (m_drawCtx)
    m_draw = std::make_unique<GtkRenderContext>(m_drawCtx, w, h, std::move(onFrameReady));
  if (m_uploadCtx)
    m_upload = std::make_unique<GtkUploadContext>(m_uploadCtx);
}

GtkContextFactory::~GtkContextFactory()
{
  m_draw.reset();
  m_upload.reset();
  if (m_drawCtx)
    g_object_unref(m_drawCtx);
  if (m_uploadCtx)
    g_object_unref(m_uploadCtx);
}

GdkGLContext * GtkContextFactory::CreateContext(GdkDisplay * display)
{
  GError * error = nullptr;
  GdkGLContext * ctx = gdk_display_create_gl_context(display, &error);
  if (!ctx)
  {
    LOG(LERROR, ("gdk_display_create_gl_context failed:", error ? error->message : "?"));
    g_clear_error(&error);
    return nullptr;
  }
  gdk_gl_context_set_allowed_apis(ctx, GDK_GL_API_GLES);
  gdk_gl_context_set_required_version(ctx, 3, 0);
  if (!gdk_gl_context_realize(ctx, &error))
  {
    LOG(LERROR, ("gdk_gl_context_realize failed:", error ? error->message : "?"));
    g_clear_error(&error);
    g_object_unref(ctx);
    return nullptr;
  }
  return ctx;
}

dp::GraphicsContext * GtkContextFactory::GetDrawContext()
{
  return m_draw.get();
}

dp::GraphicsContext * GtkContextFactory::GetResourcesUploadContext()
{
  return m_upload.get();
}

bool GtkContextFactory::DrawFrame(int64_t frameIntervalUs, std::function<void(GLuint texture)> const & draw)
{
  if (m_shutdown || !m_draw)
    return false;
  return m_draw->DrawFrame(frameIntervalUs, draw);
}
}  // namespace gtk
