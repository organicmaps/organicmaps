#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "base/assert.hpp"

#include <memory>

namespace qt
{
namespace common
{
QtOGLContextFactory::QtOGLContextFactory(QOpenGLContext * rootContext) : m_rootContext(rootContext)
{
  m_uploadSurface = CreateSurface();
  m_drawSurface = CreateSurface();
}

QtOGLContextFactory::~QtOGLContextFactory()
{
  m_drawContext.reset();
  m_uploadContext.reset();

  m_drawSurface->destroy();
  m_uploadSurface->destroy();
}

void QtOGLContextFactory::PrepareToShutdown()
{
  m_preparedToShutdown = true;
}

bool QtOGLContextFactory::AcquireFrame()
{
  if (m_preparedToShutdown || !m_drawContext)
    return false;

  return m_drawContext->AcquireFrame();
}

QRectF const & QtOGLContextFactory::GetTexRect() const
{
  ASSERT(m_drawContext != nullptr, ());
  return m_drawContext->GetTexRect();
}

GLuint QtOGLContextFactory::GetTextureHandle() const
{
  ASSERT(m_drawContext != nullptr, ());
  return m_drawContext->GetTextureHandle();
}

dp::GraphicsContext * QtOGLContextFactory::GetDrawContext()
{
  if (!m_drawContext)
    m_drawContext = std::make_unique<QtRenderOGLContext>(m_rootContext, m_drawSurface.get());

  return m_drawContext.get();
}

dp::GraphicsContext * QtOGLContextFactory::GetResourcesUploadContext()
{
  if (!m_uploadContext)
    m_uploadContext = std::make_unique<QtUploadOGLContext>(m_rootContext, m_uploadSurface.get());

  return m_uploadContext.get();
}

std::unique_ptr<QOffscreenSurface> QtOGLContextFactory::CreateSurface()
{
  QSurfaceFormat format = m_rootContext->format();
  auto result = std::make_unique<QOffscreenSurface>(m_rootContext->screen());
  result->setFormat(format);
  result->create();
  ASSERT(result->isValid(), ());

  return result;
}
}  // namespace common
}  // namespace qt
