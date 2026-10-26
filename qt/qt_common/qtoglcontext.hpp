#pragma once

#include "drape/oglcontext.hpp"

#include <QOpenGLFramebufferObject>

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

namespace qt
{
namespace common
{
class QtRenderOGLContext : public dp::OGLContext
{
public:
  QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface);

  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override;
  void Resize(uint32_t w, uint32_t h) override;

  // Called by one consumer with its shared GL context current; the frame stays valid until its next acquisition.
  bool AcquireFrame();
  GLuint GetTextureHandle() const;
  QRectF const & GetTexRect() const;
  // Called on the render thread after each new frame.
  void SetPresentListener(std::function<void()> fn);

private:
  struct Frame
  {
    std::unique_ptr<QOpenGLFramebufferObject> m_buffer;
    QRectF m_rect;
  };

  // Requires the render context; only the producer accesses the back frame and pending size/rectangle.
  void PrepareBackFrame();

  QOffscreenSurface * m_surface = nullptr;
  std::unique_ptr<QOpenGLContext> m_ctx;
  std::function<void()> m_onPresent;

  // The sampled rectangle travels with its texture through all three slots.
  Frame m_frontFrame;
  Frame m_backFrame;
  Frame m_acquiredFrame;
  QRectF m_frameRect = QRectF(0.0, 0.0, 0.0, 0.0);
  bool m_frameUpdated = false;

  std::atomic<bool> m_isContextAvailable;
  int m_width = 0;
  int m_height = 0;

  // Protects publication and swaps of the front frame between the producer and consumer.
  std::mutex m_frameMutex;
};

class QtUploadOGLContext : public dp::OGLContext
{
public:
  QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface);

  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override;

private:
  QOffscreenSurface * m_surface = nullptr;  // non-owning ptr
  std::unique_ptr<QOpenGLContext> m_ctx;
};
}  // namespace common
}  // namespace qt
