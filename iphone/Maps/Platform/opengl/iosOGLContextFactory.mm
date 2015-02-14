#import "iosOGLContextFactory.h"

iosOGLContextFactory::iosOGLContextFactory(CAEAGLLayer * layer)
  : m_layer(layer)
  , m_drawContext(NULL)
  , m_uploadContext(NULL)
{}

iosOGLContextFactory::~iosOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;
}

dp::OGLContext * iosOGLContextFactory::getDrawContext()
{
  if (m_drawContext == NULL)
    m_drawContext = new iosOGLContext(m_layer, m_uploadContext, true);
  return m_drawContext;
}

dp::OGLContext * iosOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == NULL)
    m_uploadContext = new iosOGLContext(m_layer, m_drawContext, false);
  return m_uploadContext;
}

bool iosOGLContextFactory::isDrawContextCreated() const
{
  return m_drawContext != nullptr;
}

bool iosOGLContextFactory::isUploadContextCreated() const
{
  return m_uploadContext != nullptr;
}
