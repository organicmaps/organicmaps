#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include <utility>

namespace df
{
BaseRenderer::BaseRenderer(ThreadsCommutator::ThreadName name, Params const & params)
  : m_apiVersion(params.m_apiVersion)
  , m_commutator(params.m_commutator)
  , m_contextFactory(params.m_oglContextFactory)
  , m_texMng(params.m_texMng)
  , m_threadName(name)
  , m_isEnabled(true)
  , m_renderingEnablingCompletionHandler(nullptr)
  , m_wasNotified(false)
  , m_wasContextReset(false)
{
  m_commutator->RegisterThread(m_threadName, this);
}

void BaseRenderer::StartThread()
{
  m_selfThread.Create(CreateRoutine());
}

void BaseRenderer::StopThread()
{
  // stop rendering and close queue
  m_selfThread.GetRoutine()->Cancel();
  CloseQueue();

  // wake up render thread if necessary
  if (!m_isEnabled)
  {
    WakeUp();
  }

  // wait for render thread completion
  m_selfThread.Join();
}

void BaseRenderer::SetRenderingEnabled(ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
  if (m_wasContextReset && contextFactory != nullptr)
    m_contextFactory = contextFactory;
  SetRenderingEnabled(true);
}

void BaseRenderer::SetRenderingDisabled(bool const destroyContext)
{
  if (destroyContext)
    m_wasContextReset = true;
  SetRenderingEnabled(false);
}

bool BaseRenderer::IsRenderingEnabled() const
{
  return m_isEnabled;
}

void BaseRenderer::SetRenderingEnabled(bool const isEnabled)
{
  if (isEnabled == m_isEnabled)
    return;

  // here we have to wait for completion of internal SetRenderingEnabled
  std::mutex completionMutex;
  std::condition_variable completionCondition;
  bool notified = false;
  auto handler = [&]()
  {
    std::lock_guard<std::mutex> lock(completionMutex);
    notified = true;
    completionCondition.notify_one();
  };

  {
    std::lock_guard<std::mutex> lock(m_completionHandlerMutex);
    m_renderingEnablingCompletionHandler = std::move(handler);
  }

  if (isEnabled)
  {
    // wake up rendering thread
    WakeUp();
  }
  else
  {
    // here we set up value only if rendering disabled
    m_isEnabled = false;

    // if renderer thread is waiting for message let it go
    CancelMessageWaiting();
  }

  std::unique_lock<std::mutex> lock(completionMutex);
  completionCondition.wait(lock, [&notified] { return notified; });
}

bool BaseRenderer::FilterContextDependentMessage(ref_ptr<Message> msg)
{
  return msg->IsGraphicsContextDependent();
}

void BaseRenderer::CheckRenderingEnabled()
{
  if (!m_isEnabled)
  {
    dp::GraphicsContext * context = nullptr;

    if (m_wasContextReset)
    {
      using namespace std::placeholders;
      EnableMessageFiltering(std::bind(&BaseRenderer::FilterContextDependentMessage, this, _1));
      OnContextDestroy();
    }
    else
    {
      bool const isDrawContext = m_threadName == ThreadsCommutator::RenderThread;
      context = isDrawContext ? m_contextFactory->GetDrawContext() :
                                m_contextFactory->GetResourcesUploadContext();
      context->SetRenderingEnabled(false);
    }

    OnRenderingDisabled();

    // notify initiator-thread about rendering disabling
    Notify();

    // wait for signal
    std::unique_lock<std::mutex> lock(m_renderingEnablingMutex);
    m_renderingEnablingCondition.wait(lock, [this] { return m_wasNotified; });

    m_wasNotified = false;

    bool needCreateContext = false;
    if (!m_selfThread.GetRoutine()->IsCancelled())
    {
      // here rendering is enabled again
      m_isEnabled = true;

      if (m_wasContextReset)
      {
        m_wasContextReset = false;
        needCreateContext = true;
      }
      else
      {
        context->SetRenderingEnabled(true);
      }
    }
    // notify initiator-thread about rendering enabling
    // m_renderingEnablingCompletionHandler will be setup before awakening of this thread
    Notify();

    OnRenderingEnabled();

    if (needCreateContext)
    {
      DisableMessageFiltering();
      OnContextCreate();
    }
  }
}

void BaseRenderer::Notify()
{
  std::function<void()> handler;
  {
    std::lock_guard<std::mutex> lock(m_completionHandlerMutex);
    handler = std::move(m_renderingEnablingCompletionHandler);
    m_renderingEnablingCompletionHandler = nullptr;
  }

  if (handler != nullptr)
    handler();
}

void BaseRenderer::WakeUp()
{
  std::lock_guard<std::mutex> lock(m_renderingEnablingMutex);
  m_wasNotified = true;
  m_renderingEnablingCondition.notify_one();
}

bool BaseRenderer::CanReceiveMessages()
{
  threads::IRoutine * routine = m_selfThread.GetRoutine();
  return routine != nullptr && !routine->IsCancelled();
}
}  // namespace df
