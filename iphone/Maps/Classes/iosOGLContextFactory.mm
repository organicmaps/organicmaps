#import "iosOGLContextFactory.h"

size_t constexpr kGLThreadsCount = 2;

iosOGLContextFactory::iosOGLContextFactory(CAEAGLLayer * layer, dp::ApiVersion apiVersion)
  : m_layer(layer)
  , m_apiVersion(apiVersion)
  , m_drawContext(nullptr)
  , m_uploadContext(nullptr)
  , m_isInitialized(false)
  , m_initializationCounter(0)
  , m_presentAvailable(false)
{}

iosOGLContextFactory::~iosOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;
}

dp::OGLContext * iosOGLContextFactory::getDrawContext()
{
  if (m_drawContext == nullptr)
    m_drawContext = new iosOGLContext(m_layer, m_apiVersion, m_uploadContext, true);
  return m_drawContext;
}

dp::OGLContext * iosOGLContextFactory::getResourcesUploadContext()
{
  if (m_uploadContext == nullptr)
    m_uploadContext = new iosOGLContext(m_layer, m_apiVersion, m_drawContext, false);
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

void iosOGLContextFactory::setPresentAvailable(bool available)
{
  lock_guard<mutex> lock(m_initializationMutex);
  m_presentAvailable = available;
  if (m_isInitialized)
  {
    m_drawContext->setPresentAvailable(m_presentAvailable);
  }
  else if (m_initializationCounter >= kGLThreadsCount && m_presentAvailable)
  {
    m_isInitialized = true;
    m_initializationCondition.notify_all();
  }
}

void iosOGLContextFactory::waitForInitialization(dp::OGLContext * context)
{
  unique_lock<mutex> lock(m_initializationMutex);
  if (!m_isInitialized)
  {
    m_initializationCounter++;
    if (m_initializationCounter >= kGLThreadsCount && m_presentAvailable)
    {
      m_isInitialized = true;
      m_initializationCondition.notify_all();
    }
    else
    {
      m_initializationCondition.wait(lock, [this] { return m_isInitialized; });
    }
  }

  if (m_drawContext == context)
    m_drawContext->setPresentAvailable(m_presentAvailable);
}
