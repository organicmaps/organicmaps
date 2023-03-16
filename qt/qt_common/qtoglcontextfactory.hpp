#pragma once

#include "drape/graphics_context_factory.hpp"
#include "qt/qt_common/qtoglcontext.hpp"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>

#include <memory>

namespace qt
{
namespace common
{
class QtOGLContextFactory : public dp::GraphicsContextFactory
{
public:
  QtOGLContextFactory(QOpenGLContext * rootContext);
  ~QtOGLContextFactory() override;

  void PrepareToShutdown();

  bool AcquireFrame();
  GLuint GetTextureHandle() const;
  QRectF const & GetTexRect() const;

  // dp::GraphicsContextFactory overrides:
  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;
  bool IsDrawContextCreated() const override { return m_drawContext != nullptr; }
  bool IsUploadContextCreated() const override { return m_uploadContext != nullptr; }

private:
  std::unique_ptr<QOffscreenSurface> CreateSurface();

  QOpenGLContext * m_rootContext;
  std::unique_ptr<QtRenderOGLContext> m_drawContext;
  std::unique_ptr<QOffscreenSurface> m_drawSurface;
  std::unique_ptr<QtUploadOGLContext> m_uploadContext;
  std::unique_ptr<QOffscreenSurface> m_uploadSurface;
  bool m_preparedToShutdown = false;
};
}  // namespace common
}  // namespace qt
