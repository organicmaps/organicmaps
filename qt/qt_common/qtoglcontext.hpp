#pragma once

#include "drape/oglcontext.hpp"

#include <QOpenGLFramebufferObject>

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>

#include <atomic>
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

  bool AcquireFrame();
  GLuint GetTextureHandle() const;
  QRectF const & GetTexRect() const;

private:
  QOffscreenSurface * m_surface = nullptr;
  std::unique_ptr<QOpenGLContext> m_ctx;

  std::unique_ptr<QOpenGLFramebufferObject> m_frontFrame;
  std::unique_ptr<QOpenGLFramebufferObject> m_backFrame;
  std::unique_ptr<QOpenGLFramebufferObject> m_acquiredFrame;
  QRectF m_acquiredFrameRect = QRectF(0.0, 0.0, 0.0, 0.0);
  QRectF m_frameRect = QRectF(0.0, 0.0, 0.0, 0.0);
  bool m_frameUpdated = false;

  std::atomic<bool> m_isContextAvailable;
  int m_width = 0;
  int m_height = 0;

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
