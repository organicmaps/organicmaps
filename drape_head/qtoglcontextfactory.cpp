#include "qtoglcontextfactory.hpp"

#include "../base/assert.hpp"

QtOGLContextFactory::QtOGLContextFactory(QWindow * surface)
  : m_surface(surface)
  , m_drawContext(NULL)
  , m_uploadContext(NULL)
{}

QtOGLContextFactory::~QtOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;
}

OGLContext * QtOGLContextFactory::getDrawContext()
{
  m_mutex.Lock();
  if (m_drawContext == NULL)
    m_drawContext = new QtOGLContext(m_surface, m_uploadContext);
  m_mutex.Unlock();

  return m_drawContext;
}

OGLContext * QtOGLContextFactory::getResourcesUploadContext()
{
  m_mutex.Lock();
  if (m_uploadContext == NULL)
    m_uploadContext = new QtOGLContext(m_surface, m_drawContext);
  m_mutex.Unlock();

  return m_uploadContext;
}
