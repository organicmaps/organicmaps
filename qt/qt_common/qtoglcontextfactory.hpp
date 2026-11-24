#pragma once

#include "drape/graphics_context_factory.hpp"
#include "qt/qt_common/qtoglcontext.hpp"

#include <QtGui/QOpenGLContext>

#include <functional>
#include <latch>
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
  // Register before creating the drape engine.
  // The listener runs on the render thread under the frame mutex; queue work without re-entering the context.
  void SetPresentListener(std::function<void()> fn);

  // Call from a single consumer with its shared GL context current.
  // An acquired texture stays valid until a different nonzero ID is returned or the factory is destroyed.
  // Returns zero when no texture can be acquired.
  GLuint AcquireFrameTexture();

  // dp::GraphicsContextFactory overrides:
  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;
  bool IsDrawContextCreated() const override { return m_drawContext != nullptr; }
  bool IsUploadContextCreated() const override { return m_uploadContext != nullptr; }
  void WaitForInitialization(dp::GraphicsContext * context) override;

private:
  std::unique_ptr<QOffscreenSurface> CreateSurface();

  QOpenGLContext * m_rootContext;
  std::function<void()> m_onPresent;
  std::unique_ptr<QtRenderOGLContext> m_drawContext;
  std::unique_ptr<QOffscreenSurface> m_drawSurface;
  std::unique_ptr<QtUploadOGLContext> m_uploadContext;
  std::unique_ptr<QOffscreenSurface> m_uploadSurface;
  bool m_preparedToShutdown = false;

  std::latch m_mainThreadReady = std::latch(1);
  std::latch m_contextsCreated = std::latch(2);
};
}  // namespace common
}  // namespace qt
