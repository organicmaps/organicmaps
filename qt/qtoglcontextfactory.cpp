#include "qt/qtoglcontextfactory.hpp"

#include "base/assert.hpp"

QtOGLContextFactory::QtOGLContextFactory(QWindow * surface)
  : m_surface(surface)
  , m_drawContext(nullptr)
  , m_uploadContext(nullptr)
{}

QtOGLContextFactory::~QtOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;
}

dp::OGLContext * QtOGLContextFactory::getDrawContext()
{
  if (m_drawContext == nullptr)
    m_drawContext = new QtOGLContext(m_surface, m_uploadContext);

  return m_drawContext;
}

dp::OGLContext * QtOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == nullptr)
    m_uploadContext = new QtOGLContext(m_surface, m_drawContext);

  return m_uploadContext;
}
