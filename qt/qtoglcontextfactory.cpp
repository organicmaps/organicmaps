#include "qt/qtoglcontextfactory.hpp"

#include "base/assert.hpp"

QtOGLContextFactory::QtOGLContextFactory(QOpenGLContext * renderContext, QThread * thread,
                                         TRegisterThreadFn const & regFn, TSwapFn const & swapFn)
  : m_drawContext(new QtRenderOGLContext(renderContext, thread, regFn, swapFn))
  , m_uploadContext(nullptr)
{
  m_uploadThreadSurface = new QOffscreenSurface(renderContext->screen());
  m_uploadThreadSurface->create();
}

QtOGLContextFactory::~QtOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;

  m_uploadThreadSurface->destroy();
  delete m_uploadThreadSurface;
}

dp::OGLContext * QtOGLContextFactory::getDrawContext()
{
  return m_drawContext;
}

dp::OGLContext * QtOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == nullptr)
    m_uploadContext = new QtUploadOGLContext(m_uploadThreadSurface, m_drawContext->getNativeContext());

  return m_uploadContext;
}
