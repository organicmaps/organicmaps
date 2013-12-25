#import "iosOGLContextFactory.h"

iosOGLContextFactory::iosOGLContextFactory(CAEAGLLayer * layer)
  : m_layer(layer)
  , m_drawContext(NULL)
  , m_uploadContext(NULL)
{}


OGLContext * iosOGLContextFactory::getDrawContext()
{
  if (m_drawContext == NULL)
    m_drawContext = new iosOGLContext(m_layer, m_uploadContext, true);
  return m_drawContext;
}

OGLContext * iosOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == NULL)
    m_uploadContext = new iosOGLContext(m_layer, m_drawContext, false);
  return m_uploadContext;
}
