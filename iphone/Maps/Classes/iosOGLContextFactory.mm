#import "iosOGLContextFactory.h"

size_t constexpr kGLThreadsCount = 2;

iosOGLContextFactory::iosOGLContextFactory(CAEAGLLayer * layer, dp::ApiVersion apiVersion, bool presentAvailable)
  : m_layer(layer)
  , m_apiVersion(apiVersion)
  , m_drawContext(nullptr)
  , m_uploadContext(nullptr)
  , m_isInitialized(false)
  , m_initializationCounter(0)
  , m_presentAvailable(presentAvailable)
{}

iosOGLContextFactory::~iosOGLContextFactory()
{
  delete m_drawContext;
  delete m_uploadContext;
}

dp::GraphicsContext * iosOGLContextFactory::GetDrawContext()
{
  if (m_drawContext == nullptr)
    m_drawContext = new iosOGLContext(m_layer, m_apiVersion, m_uploadContext, true /* needBuffers */);
  return m_drawContext;
}

dp::GraphicsContext * iosOGLContextFactory::GetResourcesUploadContext()
{
  if (m_uploadContext == nullptr)
    m_uploadContext = new iosOGLContext(m_layer, m_apiVersion, m_drawContext, false /* needBuffers */);
  return m_uploadContext;
}

bool iosOGLContextFactory::IsDrawContextCreated() const
{
  return m_drawContext != nullptr;
}

bool iosOGLContextFactory::IsUploadContextCreated() const
{
  return m_uploadContext != nullptr;
}

void iosOGLContextFactory::SetPresentAvailable(bool available)
{
  std::lock_guard<std::mutex> lock(m_initializationMutex);
  m_presentAvailable = available;
  if (m_isInitialized)
  {
    m_drawContext->SetPresentAvailable(m_presentAvailable);
  }
  else if (m_initializationCounter >= kGLThreadsCount && m_presentAvailable)
  {
    m_isInitialized = true;
    m_initializationCondition.notify_all();
  }
}

void iosOGLContextFactory::WaitForInitialization(dp::GraphicsContext * context)
{
  std::unique_lock<std::mutex> lock(m_initializationMutex);
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

  if (static_cast<dp::GraphicsContext *>(m_drawContext) == context)
    m_drawContext->SetPresentAvailable(m_presentAvailable);
}
