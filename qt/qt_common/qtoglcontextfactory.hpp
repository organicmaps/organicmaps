#pragma once

#include "drape/oglcontextfactory.hpp"
#include "qt/qt_common/qtoglcontext.hpp"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>

#include <memory>

namespace qt
{
namespace common
{
class QtOGLContextFactory : public dp::OGLContextFactory
{
public:
  QtOGLContextFactory(QOpenGLContext * rootContext);
  ~QtOGLContextFactory() override;

  void PrepareToShutdown();

  bool LockFrame();
  GLuint GetTextureHandle() const;
  QRectF const & GetTexRect() const;
  void UnlockFrame();

  // dp::OGLContextFactory overrides:
  dp::OGLContext * getDrawContext() override;
  dp::OGLContext * getResourcesUploadContext() override;
  bool isDrawContextCreated() const override { return m_drawContext != nullptr; }
  bool isUploadContextCreated() const override { return m_uploadContext != nullptr; }

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
