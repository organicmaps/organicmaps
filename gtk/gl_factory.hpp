#pragma once

#include "drape/gl_includes.hpp"
#include "drape/graphics_context_factory.hpp"
#include "drape/oglcontext.hpp"

#include <gdk/gdk.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

namespace gtk
{
// Offscreen colour+depth/stencil target living in the render thread's context.
// Only its texture is shared with the GTK context that composes the frame.
struct OffscreenFrame
{
  GLuint m_fbo = 0;
  GLuint m_texture = 0;
  GLuint m_depthStencil = 0;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  // Signals when the GTK context has finished reading the texture.
  GLsync m_consumerFence = nullptr;

  OffscreenFrame(uint32_t w, uint32_t h);
  ~OffscreenFrame();

  void SetConsumerFence(GLsync fence);
  // Makes the following commands of the current context wait until the consumer is done.
  void WaitForConsumer();
};

// Drape renders on its own thread into GdkGLContexts created from the display (surfaceless,
// shared with every GTK context). Frames are triple buffered, like the Qt desktop app does.
class GtkRenderContext : public dp::OGLContext
{
public:
  GtkRenderContext(GdkGLContext * ctx, uint32_t w, uint32_t h, std::function<void()> onFrameReady);
  ~GtkRenderContext() override;

  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override;
  void Resize(uint32_t w, uint32_t h) override;

  // Main thread, GTK context current: draws the newest finished frame with |draw| and fences it.
  // Returns false if there is no frame yet.
  bool DrawFrame(std::function<void(GLuint texture)> const & draw);

private:
  GdkGLContext * m_ctx;
  uint32_t m_initialWidth;
  uint32_t m_initialHeight;
  std::function<void()> m_onFrameReady;
  std::unique_ptr<OffscreenFrame> m_back;
  std::unique_ptr<OffscreenFrame> m_front;
  std::unique_ptr<OffscreenFrame> m_acquired;
  bool m_frameUpdated = false;
  bool m_hasFrame = false;
  // Held by the main thread while it draws, so frames are never swapped or deleted under it.
  std::mutex m_frameMutex;
};

class GtkUploadContext : public dp::OGLContext
{
public:
  explicit GtkUploadContext(GdkGLContext * ctx);
  ~GtkUploadContext() override;

  void Present() override {}
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer>) override {}

private:
  GdkGLContext * m_ctx;
};

class GtkContextFactory : public dp::GraphicsContextFactory
{
public:
  // Contexts must be created and realized on the main thread. |w|, |h|: initial surface size.
  GtkContextFactory(GdkDisplay * display, uint32_t w, uint32_t h, std::function<void()> onFrameReady);
  ~GtkContextFactory() override;

  bool IsValid() const { return m_drawCtx && m_uploadCtx; }

  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;
  bool IsDrawContextCreated() const override { return true; }
  bool IsUploadContextCreated() const override { return true; }

  bool DrawFrame(std::function<void(GLuint texture)> const & draw);
  void PrepareToShutdown() { m_shutdown = true; }

private:
  static GdkGLContext * CreateContext(GdkDisplay * display);

  GdkGLContext * m_drawCtx = nullptr;
  GdkGLContext * m_uploadCtx = nullptr;
  std::unique_ptr<GtkRenderContext> m_draw;
  std::unique_ptr<GtkUploadContext> m_upload;
  std::atomic<bool> m_shutdown = false;
};
}  // namespace gtk
