#pragma once

#include "drape/oglcontext.hpp"
#include "std/mutex.hpp"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLContext>

#include <memory>

namespace qt
{
namespace common
{
class QtRenderOGLContext : public dp::OGLContext
{
public:
  QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface);

  // dp::OGLContext overrides:
  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetDefaultFramebuffer() override;
  void Resize(int w, int h) override;

  void LockFrame();
  GLuint GetTextureHandle() const;
  QRectF const & GetTexRect() const;
  void UnlockFrame();

private:
  QOffscreenSurface * m_surface = nullptr;
  std::unique_ptr<QOpenGLContext> m_ctx;

  std::unique_ptr<QOpenGLFramebufferObject> m_frontFrame;
  std::unique_ptr<QOpenGLFramebufferObject> m_backFrame;
  QRectF m_texRect = QRectF(0.0, 0.0, 0.0, 0.0);

  mutex m_lock;
  bool m_resizeLock = false;
};

class QtUploadOGLContext: public dp::OGLContext
{
public:
  QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface);

  // dp::OGLContext overrides:
  void Present() override;
  void MakeCurrent() override;
  void DoneCurrent() override;
  void SetDefaultFramebuffer() override;

private:
  QOffscreenSurface * m_surface = nullptr;  // non-owning ptr
  std::unique_ptr<QOpenGLContext> m_ctx;
};
}  // namespace common
}  // namespace qt
